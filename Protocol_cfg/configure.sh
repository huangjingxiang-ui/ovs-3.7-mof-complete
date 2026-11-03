#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR"

TARGET=${1:-${OVS_SRC:-../openvswitch-3.7.0-mof}}
case "$TARGET" in
    *"openvswitch-3.7.0 (final)"*)
        echo "Refusing to modify the protected final baseline: $TARGET" >&2
        exit 2
        ;;
esac

if [ ! -f "$TARGET/configure.ac" ]; then
    echo "Not an OVS source tree: $TARGET" >&2
    echo "Usage: ./configure.sh [path-to-openvswitch-3.7.0-mof]" >&2
    exit 2
fi

# Generate C fragments and the optional ONOS/Java artifacts from the current
# protocol YAML.  A fresh Python process also resets the generator state.
mkdir -p PyPrj/cfile/packets PyPrj/cfile/odp-execute \
    PyPrj/cfile/ofproto-dpif-xlate
for dir in criteria impl packet parser mof instructions/protocol; do
    mkdir -p "PyPrj/javafile/$dir"
    find "PyPrj/javafile/$dir" -mindepth 1 -maxdepth 1 -type f -delete
done
rm -f PyPrj/javafile/instructions/Instructions.java

(cd PyPrj && python3 main.py)

# Preserve the legacy Java output path.  The C files built by append are only
# intermediate compatibility output; assemble_ovs37.py writes the OVS 3.7
# sources from the versioned 3.7 templates below.
mkdir -p tmp_file tmp_file_java
find tmp_file -mindepth 1 -maxdepth 1 -type f -delete
find tmp_file_java -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +

for dir in criteria impl packet parser mof instructions; do
    cp -R "PyPrj/javafile/$dir" tmp_file_java/
done
rm -f tmp_file_java/criteria/Criteria.java tmp_file_java/criteria/Criterion.java

cp save_file/flow_h_save1 tmp_file/flow.h
cp save_file/flow_c_save1 tmp_file/flow.c
cp save_file/packets_c_save1 tmp_file/packets.c
cp save_file/ofp-actions_c_save1 tmp_file/ofp-actions.c
cp save_file/ofproto-dpif-xlate_c_save1 tmp_file/ofproto-dpif-xlate.c
cp save_file/meta-flow_c_save tmp_file/meta-flow.c
cp save_file/odp-execute_c_save1 tmp_file/odp-execute.c
cp save_file/match_c_save1 tmp_file/match.c
cp save_file/openflow-1.0_h_save1 tmp_file/openflow-1.0.h

cp save_file/TrafficSelector_java_save1 tmp_file_java/TrafficSelector.java
cp save_file/Criterion_java_save1 tmp_file_java/criteria/Criterion.java
cp save_file/Criteria_java_save1 tmp_file_java/criteria/Criteria.java
cp save_file/DefaultTrafficSelector_java_save1 tmp_file_java/DefaultTrafficSelector.java

cc -O2 -o append append.c
./append

python3 assemble_ovs37.py --target "$TARGET"

echo "Configuration successful!"
echo "OVS 3.7-MOF target: $(cd "$TARGET" && pwd)"
echo "Generated Java files: $SCRIPT_DIR/tmp_file_java"
