#!/bin/bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
ovs_dir=${OVS_DIR:-"$script_dir/../openvswitch-3.7.0-mof"}
compound_actions_dir="$script_dir/../ovs-frontend/compound_actions"

cd "$ovs_dir"
make clean || true
./boot.sh
./configure --with-dpdk=static \
    CFLAGS="-Ofast -msse4.2 -mpopcnt -g -fno-omit-frame-pointer" \
    CPPFLAGS="-DCOMPOUND_ACTIONS_DIR='\"$compound_actions_dir\"'"
make -j"$(nproc)"
sudo make install

echo "OVS 3.7-MOF + DPDK build and install complete"
