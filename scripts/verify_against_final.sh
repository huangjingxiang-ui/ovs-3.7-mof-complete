#!/bin/bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
generated=${1:-"$script_dir/../openvswitch-3.7.0-mof"}
baseline=${2:-"/home/ovs/openvswitch-3.7.0 (final)"}

files=(
    include/openvswitch/flow.h
    lib/flow.c
    lib/packets.c
    lib/ofp-actions.c
    ofproto/ofproto-dpif-xlate.c
    lib/meta-flow.c
    lib/odp-execute.c
    lib/match.c
    include/openflow/openflow-1.0.h
    include/odp-netlink.h
    lib/meta-flow.inc
)

exact=0
different=0
for file in "${files[@]}"; do
    if cmp -s "$generated/$file" "$baseline/$file"; then
        printf 'EXACT  %s\n' "$file"
        ((exact += 1))
    elif diff -q -w -B "$generated/$file" "$baseline/$file" >/dev/null; then
        printf 'FORMAT %s\n' "$file"
        ((different += 1))
    else
        printf 'DIFF   %s\n' "$file"
        ((different += 1))
    fi
done

printf 'summary: exact=%d, non-exact=%d\n' "$exact" "$different"
test "$different" -eq 0
