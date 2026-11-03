#!/bin/bash

ovs-ctl stop

rm -rf /usr/local/var/run/openvswitch/*
rm -rf  /usr/local/etc/openvswitch/*
echo "new start" > /usr/local/var/log/openvswitch/ovs-vswitchd.log

ovs-ctl --no-ovs-vswitchd start --system-id=random
ovs-vsctl --no-wait set Open_vSwitch . other_config:pmd-cpu-mask=0xff000000ff # 0xf =  1111 1111 0000 0000 0000 0000 0000 0000 1111 1111
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-socket-mem="2048"
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
#ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-extra=--allow=0000:82:00.0
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-extra=--allow=0000:82:00.1 
ovs-ctl --no-ovsdb-server --db-sock="$DB_SOCK" start

ovs-vsctl add-br br1 -- set bridge br1 datapath_type=netdev
ovs-vsctl add-br br2 -- set bridge br2 datapath_type=netdev

ovs-vsctl add-port br1 dpdk-port1 -- set Interface dpdk-port1 type=dpdk options:dpdk-devargs=0000:82:00.1 ofport_request=1
#ovs-vsctl add-port br1 dpdk-port1 -- set Interface dpdk-port1 type=dpdk options:dpdk-devargs=0000:82:00.0 ofport_request=1
ovs-vsctl add-port br1 vhost-user2 -- set Interface vhost-user2 type=dpdkvhostuser ofport_request=2

ovs-vsctl add-port br2 vhost-user3 -- set Interface vhost-user3 type=dpdkvhostuser ofport_request=3
ovs-vsctl add-port br2 vhost-user4 -- set Interface vhost-user4 type=dpdkvhostuser ofport_request=4

ovs-vsctl add-port br2 vhost-user5 -- set Interface vhost-user5 type=dpdkvhostuser ofport_request=5
ovs-vsctl add-port br2 vhost-user6 -- set Interface vhost-user6 type=dpdkvhostuser ofport_request=6

ovs-vsctl add-port br1 vhost-user7 -- set Interface vhost-user7 type=dpdkvhostuser ofport_request=7
# 若是接受端为本机软件，则使用vhost-user8,若需要传输出去，则使用dpdk-port8
ovs-vsctl add-port br1 vhost-user8 -- set Interface vhost-user8 type=dpdkvhostuser ofport_request=8
#ovs-vsctl add-port br1 dpdk-port8 -- set Interface dpdk-port8 type=dpdk options:dpdk-devargs=0000:82:00.1 ofport_request=8

ovs-appctl vlog/set err
ovs-appctl vlog/set backtrace:dbg

ifconfig br1 10.0.1.1/24 up
ifconfig br2 10.0.2.1/24 up

ovs-ofctl del-flows br1
ovs-ofctl del-flows br2

