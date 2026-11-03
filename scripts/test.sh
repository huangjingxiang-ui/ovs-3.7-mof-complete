#!/bin/bash

pktgen -c 0x300000000000000 --proc-type auto -n 1 --socket-mem 1024 --file-prefix pktgen -a 0000:82:00.0 -- -T -P -m "57.0"

# testpmd
dpdk-testpmd -c 0x30000000 -n 1 --socket-mem 1024 \
    --file-prefix testpmd --no-pci \
    --vdev 'net_virtio_user8,mac=00:00:00:00:00:08,path=/usr/local/var/run/openvswitch/vhost-user8' \
    -- -i -a --coremask=0x20000000 --forward-mode=rxonly --stats-period 1

./build/dump -c 0x20000000 -n 1 --socket-mem 1024 \
    --file-prefix dump --no-pci \
    --vdev 'net_virtio_user8,mac=00:00:00:00:00:08,path=/usr/local/var/run/openvswitch/vhost-user8' 
