ovs-ofctl add-flow-mof br0 "in_port=vhost-user1,actions=move_protocol(dst=ipv6_i,src=ipv6_e),\
 add_protocol(ipv4_e,ipv4_e_type=0x11,\
 ip_saddr_e=192.168.0.1,\
 ip_daddr_e=192.168.1.1),\
 add_protocol(udp_pdxp,udp_source=0x8888,udp_dest=0x9999,\
 pdxp_type=0x1212,\
 pdxp_source_addr=0x11223344,\
 pdxp_dest_addr=0x55667788),\
 add_protocol(aos_1,aos_1_v_flg_vflg=0x6655,aos_1_count=0x778899),\
 output:vhost-user2"