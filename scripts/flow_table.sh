#!/bin/bash

# 流表 透传关闭

ovs-ofctl add-flow-mof br1 "priority=1,dl_type=0x0800,in_port=dpdk-port1,ip_saddr_e=192.168.0.1,ip_daddr_e=10.0.0.1,\
    actions=mod_fields(dl,dl_type=0x86dd),ipv42ipv6=0x600a0400111106ff00010000000000000000000000000001fe800000000000000000000000000001,output:vhost-user2"
ovs-ofctl add-flow-mof br2 "in_port=vhost-user3,ipv6_src_e=1::1,ipv6_dst_e=fe80::1,actions=add_bytes=0x01ff001111110411111104111111041111110401060000001102111111041102010909090909090909091dfccf1a40421111030111111111111111111111111111110f11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111114811111111111107,output:vhost-user4"
ovs-ofctl add-flow-mof br2 "dl_type=0x86dd,in_port=vhost-user5,actions=mod_fields(dl,dl_type=0x0800),ipv62ipv4=0x4500111100004000400612120a000001c0a80001,output:vhost-user6"
ovs-ofctl add-flow-mof br1 "dl_type=0x0800,in_port=vhost-user7,actions=output:vhost-user8"

./build/set_rate 1024

# 流表 透传开启

 ovs-ofctl add-flow-mof br1 "in_port=dpdk-port1,\
     actions=output:vhost-user2"
 ovs-ofctl add-flow-mof br2 "in_port=vhost-user3,\
     actions=output:vhost-user4"
 ovs-ofctl add-flow-mof br2 "in_port=vhost-user5,\
     actions=output:vhost-user6"
 ovs-ofctl add-flow-mof br1 "in_port=vhost-user7,\
     actions=output:vhost-user8"



ovs-ofctl add-flow-mof br1 "priority=1,dl_type=0x0800,in_port=dpdk-port1,actions=mod_fields(dl,dl_type=0x86dd),ipv42ipv6=0x600a0400111106ff00010000000000000000000000000001fe800000000000000000000000000001,output:vhost-user2"
ovs-ofctl add-flow-mof br2 "in_port=vhost-user3,actions=add_bytes=0x01ff001111110411111104111111041111110401060000001102111111041102010909090909090909091dfccf1a40421111030111111111111111111111111111110f11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111114811111111111107,output:vhost-user4"
ovs-ofctl add-flow-mof br2 "dl_type=0x86dd,in_port=vhost-user5,actions=mod_fields(dl,dl_type=0x0800),ipv62ipv4=0x4500111100004000400612120a000001c0a80001,output:vhost-user6"
ovs-ofctl add-flow-mof br1 "dl_type=0x0800,in_port=vhost-user7,actions=output:vhost-user8"



ovs-ofctl add-flow-mof br1 "dl_type=0x0806,in_port=dpdk-port1,actions=output:br1"
ovs-ofctl add-flow-mof br1 "dl_type=0x0806,in_port=br1,actions=output:dpdk-port1"
ovs-ofctl add-flow-mof br1 "dl_type=0x0800,ipv4_e_type=1,in_port=dpdk-port1,actions=output:br1"
ovs-ofctl add-flow-mof br1 "dl_type=0x0800,ipv4_e_type=1,in_port=br1,actions=output:dpdk-port1"
