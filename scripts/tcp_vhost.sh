#!/bin/bash

VhostPath="/root/code/multi-protocol/tcp_vhost"
cd $VhostPath || exit
ConfPath="/root/code/multi-protocol/tcp_vhost/node.conf"

nohup $VhostPath/build/server -c 0xff00000001e000 -n 1 --socket-mem 1024 \
    --file-prefix server --no-pci \
    --vdev 'net_virtio_user5,mac=00:00:00:00:00:05,path=/usr/local/var/run/openvswitch/vhost-user5' \
    --log-level 8 -- --path=$ConfPath >server.log 2>&1 &

nohup $VhostPath/build/client -c 0xff0000001f00 -n 1 --socket-mem 1024 \
    --file-prefix client --no-pci \
    --vdev 'net_virtio_user4,mac=00:00:00:00:00:04,path=/usr/local/var/run/openvswitch/vhost-user4' \
    --log-level 8 -- --path=$ConfPath >client.log 2>&1 &

nohup $VhostPath/build/link -c 0x40000000 -n 1 --socket-mem 512 \
    --file-prefix link1 --no-pci \
    --vdev 'net_virtio_user2,mac=00:00:00:00:00:02,path=/usr/local/var/run/openvswitch/vhost-user2' \
    --vdev 'net_virtio_user3,mac=00:00:00:00:00:03,path=/usr/local/var/run/openvswitch/vhost-user3' \
    --log-level 8 -- >link1.log 2>&1 &

nohup $VhostPath/build/link -c 0x80000000 -n 1 --socket-mem 512 \
    --file-prefix link2 --no-pci \
    --vdev 'net_virtio_user6,mac=00:00:00:00:00:06,path=/usr/local/var/run/openvswitch/vhost-user6' \
    --vdev 'net_virtio_user7,mac=00:00:00:00:00:07,path=/usr/local/var/run/openvswitch/vhost-user7' \
    --log-level 8 -- >link2.log 2>&1 &
