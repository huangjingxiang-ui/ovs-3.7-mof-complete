size_t parser_dl(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{
    union flow_vlan_hdr vlans[FLOW_MAX_VLAN_HEADERS];
    size_t num_vlans = parse_vlan(datap, sizep, vlans);
    VLOG_INFO("vlan label count:%d",num_vlans);

    miniflow_pad_to_64_mof((*mfp), mac_src);

    if (num_vlans > 0) {
                miniflow_push_words_32_mof((*mfp), vlan1_tpid, vlans, num_vlans);
            }

    ovs_be16 rtype = *(ovs_be16 *)data_pull(datap, sizep, sizeof(ovs_be16));
    miniflow_push_be16_mof((*mfp), dl_type, rtype);
    miniflow_pad_to_64_mof((*mfp), dl_type);
    
    g_num_vlans = num_vlans;

    return rtype;
}

size_t parser_vlan1(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), vlan1_tpid[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), vlan1_tpid[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), vlan1_qid[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), vlan1_qid[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), vlan1_qid[1]);
    return 0;
}

size_t parser_vlan2(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), vlan2_tpid[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), vlan2_tpid[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), vlan2_qid[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), vlan2_qid[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), vlan2_qid[1]);
    return 0;
}

size_t parser_sr_mpls_1(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), lse_1[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_1[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_1[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ttl_1[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), ttl_1[0]);
    return 0;
}

size_t parser_sr_mpls_2(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), lse_2[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_2[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_2[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ttl_2[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), ttl_2[0]);
    return 0;
}

size_t parser_sr_mpls_3(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), lse_3[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_3[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_3[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ttl_3[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), ttl_3[0]);
    return 0;
}

size_t parser_sr_mpls_4(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), lse_4[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_4[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_4[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ttl_4[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), ttl_4[0]);
    return 0;
}

size_t parser_sr_mpls_5(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), lse_5[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_5[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), lse_5[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ttl_5[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), ttl_5[0]);
    return 0;
}

size_t parser_arp_e(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), hw_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), hw_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    ovs_be16 rtype = *(ovs_be16 *)data_pull(datap, sizep, sizeof(ovs_be16));
    miniflow_push_be16_mof((*mfp), arp_e_type[0], rtype);
    miniflow_push_uint8_mof((*mfp), hw_len_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), proto_len_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), opcode_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), opcode_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_mac_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_mac_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_mac_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_mac_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_mac_e[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_mac_e[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_ip_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_ip_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_ip_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), src_ip_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_mac_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_mac_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_mac_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_mac_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_mac_e[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_mac_e[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_ip_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_ip_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_ip_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), dst_ip_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), dst_ip_e[3]);
    return rtype;
}

size_t parser_ipv4_e(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), ver_hl_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tos_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tot_len_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tot_len_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_id_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_id_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    data_pull(datap, sizep, sizeof(uint8_t));
    data_pull(datap, sizep, sizeof(uint8_t));
    miniflow_push_uint8_mof((*mfp), frag_off_e[0], 0);
    miniflow_push_uint8_mof((*mfp), frag_off_e[1], 0);
    miniflow_push_uint8_mof((*mfp), ttl_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    uint8_t rtype = *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t));
    miniflow_push_uint8_mof((*mfp), ipv4_e_type[0], rtype);
    miniflow_push_uint8_mof((*mfp), ip_check_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_check_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_saddr_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_saddr_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_saddr_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_saddr_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_daddr_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_daddr_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_daddr_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ip_daddr_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), ip_daddr_e[3]);
    return rtype;
}

size_t parser_ipv6_e(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), ipv6_ver_tp_flb_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_ver_tp_flb_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_ver_tp_flb_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_ver_tp_flb_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_plen_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_plen_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    uint8_t rtype_6 = *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t));
    miniflow_push_uint8_mof((*mfp), ipv6_e_type[0], rtype_6);
    miniflow_push_uint8_mof((*mfp), ipv6_hlmt_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[6], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[7], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[8], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[9], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[10], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[11], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[12], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[13], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[14], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_src_e[15], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[6], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[7], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[8], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[9], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[10], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[11], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[12], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[13], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[14], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ipv6_dst_e[15], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    return rtype_6;
}

size_t parser_tcp(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), tcp_source[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tcp_source[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tcp_dest[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tcp_dest[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), seq[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), seq[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), seq[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), seq[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ack_seq[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ack_seq[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ack_seq[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), ack_seq[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), off_bits[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), off_bits[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), window[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), window[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tcp_check[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), tcp_check[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), urg_ptr[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), urg_ptr[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_pad_to_64_mof((*mfp), urg_ptr[1]);
    return 0;
}

size_t parser_udp(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), udp_source[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), udp_source[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), udp_dest[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), udp_dest[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), len[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), len[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), udp_check[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), udp_check[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    return 0;
}

size_t parser_icmpv6(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    miniflow_push_uint8_mof((*mfp), v6_Type[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), v6_code[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), v6_checksum[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), v6_checksum[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), v6_flags[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), v6_flags[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), v6_flags[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), v6_flags[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[6], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[7], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[8], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[9], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[10], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[11], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[12], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[13], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[14], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_target[15], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    uint8_t rtype_icmp = *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t));
    miniflow_push_uint8_mof((*mfp), icmpv6_type[0], rtype_icmp);
    miniflow_push_uint8_mof((*mfp), icmpv6_length[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_dst_mac[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_dst_mac[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_dst_mac[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_dst_mac[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_dst_mac[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), nd_dst_mac[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    return rtype_icmp;
}

size_t parser_srv6(void **datap, uint32_t *sizep, struct mf_ctx *mfp)
{ 
    uint8_t rtype_s6 = *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t));
    miniflow_push_uint8_mof((*mfp), srv6_type[0], rtype_s6);
    uint8_t len = *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t));
    miniflow_push_uint8_mof((*mfp), srv6_hdr_ext_len, len);
    miniflow_push_uint8_mof((*mfp), srv6_routing_Type[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), srv6_segments_left[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), srv6_last_enty[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), srv6_flags[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), srv6_tag[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    miniflow_push_uint8_mof((*mfp), srv6_tag[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
    uint8_t segsize = (len-8) / 16;
    int i = 0;
    srv6num = segsize;
    if(i < segsize) {
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[6], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[7], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[8], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[9], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[10], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[11], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[12], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[13], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[14], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist1.eb[15], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        i++;
    }
    if(i < segsize) {
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[6], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[7], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[8], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[9], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[10], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[11], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[12], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[13], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[14], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist2.eb[15], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        i++;
    }
    if(i < segsize) {
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[0], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[1], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[2], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[3], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[4], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[5], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[6], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[7], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[8], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[9], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[10], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[11], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[12], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[13], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[14], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        miniflow_push_uint8_mof((*mfp), srv6_segmentlist3.eb[15], *(uint8_t *)data_pull(datap, sizep, sizeof(uint8_t)));
        i++;
    }
    return rtype_s6;
}

#define dl_TYPE_ipv4_e ntohs(0x0800)
#define dl_TYPE_ipv6_e ntohs(0x86DD)
#define dl_TYPE_arp_e ntohs(0x0806)
#define dl_TYPE_sr_mpls_1 ntohs(0x8847)
#define ipv4_e_TYPE_tcp 0x06
#define ipv4_e_TYPE_udp 0x11
#define ipv6_e_TYPE_tcp 0x06
#define ipv6_e_TYPE_udp 0x11
#define ipv6_e_TYPE_icmpv6 0x3A
#define ipv6_e_TYPE_srv6 0x12
#define srv6_TYPE_tcp 0x06
#define srv6_TYPE_udp 0x11


void miniflow_extract_mof(struct dp_packet *packet,struct miniflow *dst)
{
    BUILD_ASSERT_DECL(FLOW_WC_SEQ == 43);
    
    const struct pkt_metadata *md = &packet->md;
    void *data = dp_packet_data(packet);
    size_t size = dp_packet_size(packet);
    ovs_be32 packet_type = packet->packet_type;
    uint64_t *values = miniflow_values(dst);
    struct mf_ctx mf = { FLOWMAP_EMPTY_INITIALIZER, values,values + FLOW_U64S_mof };
    const char *frame;
    uint8_t nw_frag, nw_tos, nw_ttl, nw_proto;
    uint8_t *ct_nw_proto_p = NULL;
    ovs_be16 ct_tp_src = 0, ct_tp_dst = 0;
    int srv6_flags[3]={SRV6_1,SRV6_2,SRV6_3,};
    packet->flag = 0;

    /* Metadata. */ 
    if (flow_tnl_dst_is_set(&md->tunnel)) {
        miniflow_push_words(mf, tunnel, &md->tunnel,
                            offsetof(struct flow_tnl, metadata) /
                            sizeof(uint64_t));

        if (!(md->tunnel.flags & FLOW_TNL_F_UDPIF)) {
            if (md->tunnel.metadata.present.map) {
                miniflow_push_words(mf, tunnel.metadata, &md->tunnel.metadata,
                                    sizeof md->tunnel.metadata /
                                    sizeof(uint64_t));
            }
        } else {
            if (md->tunnel.metadata.present.len) {
                miniflow_push_words(mf, tunnel.metadata.present,
                                    &md->tunnel.metadata.present, 1);
                miniflow_push_words(mf, tunnel.metadata.opts.gnv,
                                    md->tunnel.metadata.opts.gnv,
                                    DIV_ROUND_UP(md->tunnel.metadata.present.len,
                                                 sizeof(uint64_t)));
            }
        }
    }
    if (md->skb_priority || md->pkt_mark) {
        miniflow_push_uint32(mf, skb_priority, md->skb_priority);
        miniflow_push_uint32(mf, pkt_mark, md->pkt_mark);
    }
    miniflow_push_uint32(mf, dp_hash, md->dp_hash);
    miniflow_push_uint32(mf, in_port, odp_to_u32(md->in_port.odp_port));
    if (md->ct_state) {
        miniflow_push_uint32(mf, recirc_id, md->recirc_id);
        miniflow_push_uint8(mf, ct_state, md->ct_state);
        ct_nw_proto_p = miniflow_pointer(mf, ct_nw_proto);
        miniflow_push_uint8(mf, ct_nw_proto, 0);
        miniflow_push_uint16(mf, ct_zone, md->ct_zone);
        miniflow_push_uint32(mf, ct_mark, md->ct_mark);
        miniflow_push_be32(mf, packet_type, packet_type);
        if (!ovs_u128_is_zero(md->ct_label)) {
            miniflow_push_words(mf, ct_label, &md->ct_label,
                                sizeof md->ct_label / sizeof(uint64_t));
        }
    } else {
        if (md->recirc_id) {
            miniflow_push_uint32(mf, recirc_id, md->recirc_id);
            miniflow_pad_to_64(mf, recirc_id);
        }
        miniflow_pad_from_64(mf, packet_type);
        miniflow_push_be32(mf, packet_type, packet_type);
    }

    /* Initialize packet's layer pointer and offsets. */
    frame = data;
    dp_packet_reset_offsets(packet);

    VLOG_INFO("size of flow is %d",sizeof(struct flow));
    VLOG_INFO("size of flow_mof is %d",sizeof(struct flow_mof));
    VLOG_INFO("****************Before MODIFY********************");
    uint8_t *pa_data = dp_packet_data(packet);
    
    for(int i = 0; i < 200; i += 8) {
        VLOG_INFO("package data[%d] (8Bytes):%x %x %x %x %x %x %x %x",i/8,pa_data[i],pa_data[i+1],pa_data[i+2],pa_data[i+3],pa_data[i+4],pa_data[i+5],pa_data[i+6],pa_data[i+7]);
    }

    miniflow_push_macs(mf, dl_dst, data);
    packet->l3_ofs = 14;
    packet->l4_ofs = 14;

    VLOG_INFO("enter dl parser");
    ovs_be16 dl_type = parser_dl(&data, &size, &mf);
    packet->flag |= DL;
    packet->l3_ofs = (char *) data - frame;
    if (dl_type == dl_TYPE_ipv4_e)
    {
        VLOG_INFO("enter ipv4_e parser");
        uint8_t ipv4_e_type = parser_ipv4_e(&data, &size, &mf);
        packet->flag |= IPV4_E;
        packet->l4_ofs = (char *) data - frame;
        if (ipv4_e_type == ipv4_e_TYPE_tcp)
        {
            VLOG_INFO("enter tcp parser");
            parser_tcp(&data, &size, &mf);
            packet->flag |= TCP;
        }
        else if (ipv4_e_type == ipv4_e_TYPE_udp)
        {
            VLOG_INFO("enter udp parser");
            parser_udp(&data, &size, &mf);
            packet->flag |= UDP;
        }
    }
    else if (dl_type == dl_TYPE_ipv6_e)
    {
        VLOG_INFO("enter ipv6_e parser");
        uint8_t ipv6_e_type = parser_ipv6_e(&data, &size, &mf);
        packet->flag |= IPV6_E;
        packet->l4_ofs = (char *) data - frame;
        if (ipv6_e_type == ipv6_e_TYPE_tcp)
        {
            VLOG_INFO("enter tcp parser");
            parser_tcp(&data, &size, &mf);
            packet->flag |= TCP;
        }
        else if (ipv6_e_type == ipv6_e_TYPE_udp)
        {
            VLOG_INFO("enter udp parser");
            parser_udp(&data, &size, &mf);
            packet->flag |= UDP;
        }
        else if (ipv6_e_type == ipv6_e_TYPE_icmpv6)
        {
            VLOG_INFO("enter icmpv6 parser");
            uint8_t icmpv6_type = parser_icmpv6(&data, &size, &mf);
            packet->flag |= ICMPV6;
        }
        else if (ipv6_e_type == ipv6_e_TYPE_srv6)
        {
            VLOG_INFO("enter srv6 parser");
            uint8_t srv6_type = parser_srv6(&data, &size, &mf);
            packet->flag |= srv6_flags[srv6num];
            if (srv6_type == srv6_TYPE_tcp)
            {
                VLOG_INFO("enter tcp parser");
                parser_tcp(&data, &size, &mf);
                packet->flag |= TCP;
            }
            else if (srv6_type == srv6_TYPE_udp)
            {
                VLOG_INFO("enter udp parser");
                parser_udp(&data, &size, &mf);
                packet->flag |= UDP;
            }
        }
    }
    else if (dl_type == dl_TYPE_arp_e)
    {
        VLOG_INFO("enter arp_e parser");
        ovs_be16 arp_e_type = parser_arp_e(&data, &size, &mf);
        packet->flag |= ARP_E;
        packet->l4_ofs = (char *) data - frame;
    }
    else if (dl_type == dl_TYPE_sr_mpls_1)
    {
        VLOG_INFO("enter sr_mpls_1 parser");
        parser_sr_mpls_1(&data, &size, &mf);
        packet->flag |= SR_MPLS_1;
        packet->l4_ofs = (char *) data - frame;
    }
    if(g_num_vlans == 1){
        packet->flag |= VLAN1;
    }
    if(g_num_vlans == 2){
        packet->flag |= VLAN1;
        packet->flag |= VLAN2;
    }
    packet->flag |= MAC;

    dst->map = mf.map;
}

uint32_t miniflow_hash(const struct miniflow *flow)
{
	uint32_t hash = 0;

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, dl_type[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, dl_type[1]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_1[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_1[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_1[2]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_2[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_2[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_2[2]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_3[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_3[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_3[2]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_4[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_4[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_4[2]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_5[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_5[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, lse_5[2]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, src_ip_e[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, src_ip_e[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, src_ip_e[2]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, src_ip_e[3]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, dst_ip_e[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, dst_ip_e[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, dst_ip_e[2]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, dst_ip_e[3]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_saddr_e[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_saddr_e[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_saddr_e[2]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_saddr_e[3]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_daddr_e[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_daddr_e[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_daddr_e[2]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, ip_daddr_e[3]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, tcp_source[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, tcp_source[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, tcp_dest[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, tcp_dest[1]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, udp_source[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, udp_source[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, udp_dest[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, udp_dest[1]));

    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[2]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[3]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[4]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[5]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[6]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[7]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[8]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[9]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[10]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[11]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[12]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[13]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[14]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_target[15]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_dst_mac[0]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_dst_mac[1]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_dst_mac[2]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_dst_mac[3]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_dst_mac[4]));
    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, nd_dst_mac[5]));
    return hash_finish(hash, 42);
}
