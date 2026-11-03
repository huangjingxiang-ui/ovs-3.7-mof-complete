## ovs终端 ##
ovs-ctl stop
rm /usr/local/etc/openvswitch/*
rm /usr/local/var/run/openvswitch/*
rm /usr/local/var/log/openvswitch/*

ovs-ctl --no-ovs-vswitchd start --system-id=random
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-lcore-mask=0x1
ovs-vsctl --no-wait set Open_vSwitch . other_config:pmd-cpu-mask=0xf0
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-socket-mem="1024"
ovs-vsctl --no-wait set Open_vSwitch . other_config:dpdk-init=true
#ovs-vsctl --no-wait set open_vswitch . other_config:vhost-postcopy-support=true
ovs-ctl --no-ovsdb-server --db-sock="$DB_SOCK" start

#ovs-appctl vlog/set off
ovs-appctl vlog/set info


