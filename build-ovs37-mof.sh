#!/usr/bin/env bash
# Reconstruct and compile OVS 3.7-MOF with DPDK 23.11 on Ubuntu.
#
# Default behaviour is deliberately non-destructive: generate, configure and
# compile only.  It does not stop the running OVS instance or install files.

set -Eeuo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
OVS_DIR=${OVS_DIR:-"$SCRIPT_DIR/openvswitch-3.7.0-mof"}
PROTOCOL_DIR=${PROTOCOL_DIR:-"$SCRIPT_DIR/Protocol_cfg"}
COMPOUND_ACTIONS_DIR=${COMPOUND_ACTIONS_DIR:-"$SCRIPT_DIR/ovs-frontend/compound_actions"}
BUILD_DIR=${BUILD_DIR:-""}
DPDK_LINK=${DPDK_LINK:-shared}
JOBS=${JOBS:-"$(nproc)"}

DO_GENERATE=1
DO_CLEAN=0
DO_INSTALL=0
DO_CHECK=0
FORCE_CONFIGURE=0

usage() {
    cat <<'EOF'
Usage: ./build-ovs37-mof.sh [options]

Reconstruct OVS 3.7-MOF from the current YAML and compile it with DPDK 23.11.
The default action is: generate -> configure -> make -> binary verification.

Options:
  --ovs-dir PATH       OVS 3.7 source tree (default: ./openvswitch-3.7.0-mof)
  --build-dir PATH     Build directory (default: same as --ovs-dir)
  --jobs N             Parallel build jobs (default: number of CPUs)
  --clean              Run make distclean before configuring
  --reconfigure        Run configure even if config.status already exists
  --skip-generate      Do not run Protocol_cfg/configure.sh
  --check              Run make check after compilation
  --install            Run sudo make install after a successful build
  -h, --help           Show this help

Environment overrides:
  OVS_DIR, BUILD_DIR, PROTOCOL_DIR, COMPOUND_ACTIONS_DIR, JOBS,
  PKG_CONFIG_PATH, CFLAGS, CPPFLAGS and DPDK_LINK (shared or static).

Examples:
  ./build-ovs37-mof.sh
  ./build-ovs37-mof.sh --clean --jobs 2
  ./build-ovs37-mof.sh --check
  ./build-ovs37-mof.sh --install
EOF
}

die() {
    echo "ERROR: $*" >&2
    exit 1
}

step() {
    echo
    echo "========== $* =========="
}

while (($#)); do
    case "$1" in
        --ovs-dir)
            (($# >= 2)) || die "--ovs-dir requires a path"
            OVS_DIR=$2
            shift 2
            ;;
        --build-dir)
            (($# >= 2)) || die "--build-dir requires a path"
            BUILD_DIR=$2
            shift 2
            ;;
        --jobs)
            (($# >= 2)) || die "--jobs requires a number"
            JOBS=$2
            shift 2
            ;;
        --clean)
            DO_CLEAN=1
            FORCE_CONFIGURE=1
            shift
            ;;
        --reconfigure)
            FORCE_CONFIGURE=1
            shift
            ;;
        --skip-generate)
            DO_GENERATE=0
            shift
            ;;
        --check)
            DO_CHECK=1
            shift
            ;;
        --install)
            DO_INSTALL=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "unknown option: $1 (use --help)"
            ;;
    esac
done

[[ "$JOBS" =~ ^[1-9][0-9]*$ ]] || die "--jobs must be a positive integer"
OVS_DIR=$(realpath -e -- "$OVS_DIR")
PROTOCOL_DIR=$(realpath -e -- "$PROTOCOL_DIR")
BUILD_DIR=$(realpath -m -- "${BUILD_DIR:-$OVS_DIR}")
case "$DPDK_LINK" in shared|static) ;; *) die 'DPDK_LINK must be shared or static' ;; esac

case "$OVS_DIR|$BUILD_DIR" in
    *"openvswitch-3.7.0 (final)"*)
        die "the protected final baseline must not be generated or compiled in place: $OVS_DIR"
        ;;
esac

[[ -f "$OVS_DIR/configure.ac" ]] || die "OVS source tree not found: $OVS_DIR"
[[ -f "$PROTOCOL_DIR/configure.sh" ]] || die "generator not found: $PROTOCOL_DIR/configure.sh"
[[ -d "$COMPOUND_ACTIONS_DIR" ]] || die "compound-actions directory not found: $COMPOUND_ACTIONS_DIR"

for command_name in python3 gcc make pkg-config strings; do
    command -v "$command_name" >/dev/null 2>&1 || die "required command not found: $command_name"
done

DPDK_VERSION=$(pkg-config --modversion libdpdk 2>/dev/null) || {
    die "libdpdk was not found by pkg-config; install DPDK 23.11 or set PKG_CONFIG_PATH"
}
case "$DPDK_VERSION" in
    23.11|23.11.*) ;;
    *) die "DPDK 23.11 is required, but pkg-config reports $DPDK_VERSION" ;;
esac

step "Environment"
echo "Project       : $SCRIPT_DIR"
echo "OVS source    : $OVS_DIR"
echo "Build dir     : $BUILD_DIR"
echo "DPDK          : $DPDK_VERSION"
echo "Parallel jobs : $JOBS"

if ((DO_GENERATE)); then
    step "Generate OVS 3.7-MOF sources from YAML"
    bash "$PROTOCOL_DIR/configure.sh" "$OVS_DIR"
fi

mkdir -p "$BUILD_DIR"

if ((DO_CLEAN)); then
    step "Clean previous build"
    if [[ -f "$BUILD_DIR/Makefile" ]]; then
        make -C "$BUILD_DIR" distclean
    fi
fi

if [[ ! -x "$OVS_DIR/configure" ]]; then
    step "Generate configure script"
    (cd "$OVS_DIR" && bash ./boot.sh)
fi

if ((FORCE_CONFIGURE)) || [[ ! -f "$BUILD_DIR/config.status" ]]; then
    step "Configure OVS 3.7-MOF with DPDK 23.11"

    DEFAULT_CFLAGS="-O2 -g -msse4.2 -mpopcnt -fno-omit-frame-pointer"
    DEFAULT_CPPFLAGS="-DCOMPOUND_ACTIONS_DIR='\"$COMPOUND_ACTIONS_DIR\"'"

    (
        cd "$BUILD_DIR"
        CFLAGS=${CFLAGS:-$DEFAULT_CFLAGS} \
        CPPFLAGS=${CPPFLAGS:-$DEFAULT_CPPFLAGS} \
        "$OVS_DIR/configure" --with-dpdk="$DPDK_LINK"
    )
else
    step "Reuse existing configure result"
    echo "Use --reconfigure if DPDK, compiler flags or paths have changed."
fi

step "Compile OVS 3.7-MOF"
make -C "$BUILD_DIR" -j"$JOBS"

if ((DO_CHECK)); then
    step "Run source test suite"
    make -C "$BUILD_DIR" -j"$JOBS" check
fi

step "Verify build result"
VSWITCHD="$BUILD_DIR/vswitchd/ovs-vswitchd"
OFCTL="$BUILD_DIR/utilities/ovs-ofctl"

[[ -x "$VSWITCHD" ]] || die "ovs-vswitchd was not produced: $VSWITCHD"
[[ -x "$OFCTL" ]] || die "ovs-ofctl was not produced: $OFCTL"

"$VSWITCHD" --version

for mof_command in add-flow-mof del-flows-mof dump-flows-mof; do
    if ! strings "$OFCTL" | grep -Fx "$mof_command" >/dev/null; then
        die "built ovs-ofctl does not contain command: $mof_command"
    fi
done

if ((DO_INSTALL)); then
    step "Install OVS 3.7-MOF"
    sudo make -C "$BUILD_DIR" install
fi

echo
echo "Build completed successfully."
echo "ovs-vswitchd: $VSWITCHD"
echo "ovs-ofctl   : $OFCTL"
if ((!DO_INSTALL)); then
    echo "Nothing was installed. Re-run with --install when installation is required."
fi
