#!/bin/bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
ovs_dir=${OVS_DIR:-"$script_dir/../openvswitch-3.7.0-mof"}

cd "$ovs_dir"
if [ -x /usr/local/share/openvswitch/scripts/ovs-ctl ]; then
    sudo /usr/local/share/openvswitch/scripts/ovs-ctl stop || true
fi
make -j"$(nproc)"
sudo make install

echo "OVS 3.7-MOF incremental build and install complete"
