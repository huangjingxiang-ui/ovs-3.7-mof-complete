import yaml
import json
import createflow
import createparser
align = "    "
pasermetadata = align + '/* Metadata. */ \n\
    if (flow_tnl_dst_is_set(&md->tunnel)) {\n\
        miniflow_push_words(mf, tunnel, &md->tunnel,\n\
                            offsetof(struct flow_tnl, metadata) /\n\
                            sizeof(uint64_t));\n\
\n\
        if (!(md->tunnel.flags & FLOW_TNL_F_UDPIF)) {\n\
            if (md->tunnel.metadata.present.map) {\n\
                miniflow_push_words(mf, tunnel.metadata, &md->tunnel.metadata,\n\
                                    sizeof md->tunnel.metadata /\n\
                                    sizeof(uint64_t));\n\
            }\n\
        } else {\n\
            if (md->tunnel.metadata.present.len) {\n\
                miniflow_push_words(mf, tunnel.metadata.present,\n\
                                    &md->tunnel.metadata.present, 1);\n\
                miniflow_push_words(mf, tunnel.metadata.opts.gnv,\n\
                                    md->tunnel.metadata.opts.gnv,\n\
                                    DIV_ROUND_UP(md->tunnel.metadata.present.len,\n\
                                                 sizeof(uint64_t)));\n\
            }\n\
        }\n\
    }\n\
    if (md->skb_priority || md->pkt_mark) {\n\
        miniflow_push_uint32(mf, skb_priority, md->skb_priority);\n\
        miniflow_push_uint32(mf, pkt_mark, md->pkt_mark);\n\
    }\n\
    miniflow_push_uint32(mf, dp_hash, md->dp_hash);\n\
    miniflow_push_uint32(mf, in_port, odp_to_u32(md->in_port.odp_port));\n\
    if (md->ct_state) {\n\
        // VLOG_INFO("ct_state exsists");\n\
        miniflow_push_uint32(mf, recirc_id, md->recirc_id);\n\
        miniflow_push_uint8(mf, ct_state, md->ct_state);\n\
        ct_nw_proto_p = miniflow_pointer(mf, ct_nw_proto);\n\
        miniflow_push_uint8(mf, ct_nw_proto, 0);\n\
        miniflow_push_uint16(mf, ct_zone, md->ct_zone);\n\
        miniflow_push_uint32(mf, ct_mark, md->ct_mark);\n\
        miniflow_push_be32(mf, packet_type, packet_type);\n\
        if (!ovs_u128_is_zero(md->ct_label)) {\n\
            miniflow_push_words(mf, ct_label, &md->ct_label,\n\
                                sizeof md->ct_label / sizeof(uint64_t));\n\
        }\n\
    } else {\n\
        // VLOG_INFO("ct_state not exsists");\n\
        if (md->recirc_id) {\n\
            // VLOG_INFO("recirc_id exsists");\n\
            miniflow_push_uint32(mf, recirc_id, md->recirc_id);\n\
            miniflow_pad_to_64(mf, recirc_id);\n\
        }\n\
        miniflow_pad_from_64(mf, packet_type);\n\
        miniflow_push_be32(mf, packet_type, packet_type);\n\
    }\n\
'


def get_if(tp_name, value, cnt):
    return align*cnt + 'if (' + tp_name + ' == ' + value + ')\n'


def get_elif(tp_name, value, cnt):
    return align*cnt + 'else if (' + tp_name + ' == ' + value + ')\n'


# def get_parser(name, size, cnt):
#     name_ = name[0:len(name)-len('_type')]
#     if (name_ == 'srv6'):
#         return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + size + ' ' + name + ' = ' + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + '''packet->flag |= srv6_flags[srv6num];''' + '\n'
#     else:
#         return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + size + ' ' + name + ' = ' + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + 'packet->flag |= ' + (name[:-5]).upper() + ';\n'


# def get_parser_without_type(name, cnt):
#     name_ = name[0:len(name)-len('_type')]
#     if (name_ == 'srv6'):
#         return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + '''packet->flag |= srv6_flags[srv6num];''' + '\n'
#     else:
#         return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + 'packet->flag |= ' + (name[:-5]).upper() + ';\n'

#############################################################

# 修改两函数为如下，只修改了l3和l4的ofs：
# def get_parser(name, size, cnt):
#     name_ = name[0:len(name)-len('_type')]
#     layer2_e = []
#     layer3 = []
#     with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
#         res = yaml.safe_load(f)
#         layer2_e = res.get('Layer2_e', []) if isinstance(res, dict) else []
#         layer3 = res.get('Layer3', []) if isinstance(res, dict) else []
#         
#     if (name_ == 'srv6'):
#         return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + size + ' ' + name + ' = ' + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + '''packet->flag |= srv6_flags[srv6num];''' + '\n'
#     else:
#         # return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#         ans = align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + size + ' ' + name + ' = ' + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + 'packet->flag |= ' + (name[:-5]).upper() + ';\n'
#         if name_ in layer2_e:
#             ans += align*cnt + 'packet->l3_ofs = (char *) data - frame;\n'
#         elif name_ in layer3:
#             ans += align*cnt + 'packet->l4_ofs = (char *) data - frame;\n'
#         return ans
#
# def get_parser_without_type(name, cnt):
#     name_ = name[0:len(name)-len('_type')]
#     layer2_e = []
#     layer3 = []
#     with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
#         res = yaml.safe_load(f)
#         layer2_e = res.get('Layer2_e', []) if isinstance(res, dict) else []
#         layer3 = res.get('Layer3', []) if isinstance(res, dict) else []
#
#     if (name_ == 'srv6'):
#         return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + '''packet->flag |= srv6_flags[srv6num];''' + '\n'
#     else:
#          # return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#         ans = align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
#             + align*cnt + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
#             + align*cnt + 'packet->flag |= ' + (name[:-5]).upper() + ';\n'
#         if name_ in layer2_e:
#             ans += align*cnt + 'packet->l3_ofs = (char *) data - frame;\n'
#         elif name_ in layer3:
#             ans += align*cnt + 'packet->l4_ofs = (char *) data - frame;\n'
#         return ans

def get_parser(name, size, cnt):
    import re
    import yaml
    name_ = name[0:len(name)-len('_type')]
    
    if (name_ == 'srv6'):
        return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
            + align*cnt + size + ' ' + name + ' = ' + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
            + align*cnt + '''packet->flag |= srv6_flags[srv6num];''' + '\n'

    ans = align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
        + align*cnt + size + ' ' + name + ' = ' + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
        + align*cnt + 'packet->flag |= ' + (name[:-5]).upper() + ';\n'

    with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
        res = yaml.safe_load(f)
        if isinstance(res, dict):
            max_layer = 0
            for k in res.keys():
                m = re.match(r'Layer(\d+)', k)
                if m:
                    max_layer = max(max_layer, int(m.group(1)))
            
            for key, protocols in res.items():
                if isinstance(protocols, list) and name_ in protocols:
                    if key == 'Layer2_e':
                        if max_layer >= 3:
                            ans += align*cnt + 'packet->l3_ofs = (char *) data - frame;\n'
                    else:
                        m = re.match(r'Layer(\d+)', key)
                        if m:
                            layer_num = int(m.group(1))
                            if layer_num >= 3 and layer_num < max_layer:
                                ans += align*cnt + f'packet->l{layer_num + 1}_ofs = (char *) data - frame;\n'
                    break
    return ans

def get_parser_without_type(name, cnt):
    import re
    import yaml
    name_ = name[0:len(name)-len('_type')]

    if (name_ == 'srv6'):
        return align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
            + align*cnt + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
            + align*cnt + '''packet->flag |= srv6_flags[srv6num];''' + '\n'

    ans = align*cnt + 'VLOG_INFO("enter ' + (name[:-5]) + ' parser");\n'\
        + align*cnt + 'parser_' + name_ + ('(&data, &size, &mf);\n')\
        + align*cnt + 'packet->flag |= ' + (name[:-5]).upper() + ';\n'

    with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
        res = yaml.safe_load(f)
        if isinstance(res, dict):
            max_layer = 0
            for k in res.keys():
                m = re.match(r'Layer(\d+)', k)
                if m:
                    max_layer = max(max_layer, int(m.group(1)))
                    
            for key, protocols in res.items():
                if isinstance(protocols, list) and name_ in protocols:
                    if key == 'Layer2_e':
                        if max_layer >= 3:
                            ans += align*cnt + 'packet->l3_ofs = (char *) data - frame;\n'
                    else:
                        m = re.match(r'Layer(\d+)', key)
                        if m:
                            layer_num = int(m.group(1))
                            if layer_num >= 3 and layer_num < max_layer:
                                ans += align*cnt + f'packet->l{layer_num + 1}_ofs = (char *) data - frame;\n'
                    break
    return ans

#############################################################

def ret_srv6_flags(srv6num):
    STRSRV6 = "int srv6_flags[" + str(srv6num) + ']={' 
    for i in range(srv6num):
        STRSRV6 += 'SRV6_' + str(i+1) + ','
    STRSRV6 += '};'
    return STRSRV6
    
##################################################################

# 只修改了l3和l4的ofs
# def extract_init(fo,srv6num):
#     fo.write(
#         '\n\nvoid miniflow_extract_mof(struct dp_packet *packet,struct miniflow *dst)\n'
#         + "{\n"
#         + align+"BUILD_ASSERT_DECL(FLOW_WC_SEQ == 42);\n"
#         + align+"\n"
#         + align+"const struct pkt_metadata *md = &packet->md;\n"
#         + align+"void *data = dp_packet_data(packet);\n"
#         + align+"size_t size = dp_packet_size(packet);\n"
#         + align+"ovs_be32 packet_type = packet->packet_type;\n"
#         + align+'uint64_t *values = miniflow_values(dst);\n'
#         + align +
#         "struct mf_ctx mf = { FLOWMAP_EMPTY_INITIALIZER, values,values + FLOW_U64S_mof };\n"
#         + align+"const char *frame;\n"
#         + align+'uint8_t nw_frag, nw_tos, nw_ttl, nw_proto;\n'
#         + align+'uint8_t *ct_nw_proto_p = NULL;\n'
#         + align+'ovs_be16 ct_tp_src = 0, ct_tp_dst = 0;\n'
#         + align + ret_srv6_flags(srv6num) + '\n'
#         + align + 'packet->flag = 0;\n'
#         + '\n'
#         + pasermetadata + '\n'
#         + align + '''// uint8_t *bi = mf.map.bits;
#     // VLOG_INFO("bits[0] after metadata push:%x %x %x %x %x %x %x %x",bi[7],bi[6],bi[5],bi[4],bi[3],bi[2],bi[1],bi[0]);
#
#     /* Initialize packet's layer pointer and offsets. */
#     frame = data;
#     dp_packet_reset_offsets(packet);
#
#
#     VLOG_INFO("size of flow is %d",sizeof(struct flow));
#     VLOG_INFO("size of flow_mof is %d",sizeof(struct flow_mof));
#     VLOG_INFO("****************Before MODIFY********************");
#     uint8_t *pa_data = dp_packet_data(packet);
#     
#     for(int i = 0; i < 200; i += 8) {
#         VLOG_INFO("package data[%d] (8Bytes):%x %x %x %x %x %x %x %x",i/8,pa_data[i],pa_data[i+1],pa_data[i+2],pa_data[i+3],pa_data[i+4],pa_data[i+5],pa_data[i+6],pa_data[i+7]);
#     }
#
#     //VLOG_INFO("flow_modify");
#
#     //flow_modify((uint8_t *)data);
#     
#     miniflow_push_macs(mf, dl_dst, data);
#     packet->l3_ofs = 14;
#     packet->l4_ofs = 14;\n\n'''
#     )

def extract_init(fo,srv6num):
    import yaml
    import re
    max_layer = 3
    with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
        res = yaml.safe_load(f)
        if isinstance(res, dict):
            for key in res.keys():
                m = re.match(r'Layer(\d+)', key)
                if m:
                    max_layer = max(max_layer, int(m.group(1)))
                    
    ofs_inits = ""
    for i in range(3, max_layer + 1):
        ofs_inits += align + f'packet->l{i}_ofs = 14;\n'

    fo.write(
        '\n\nvoid miniflow_extract_mof(struct dp_packet *packet,struct miniflow *dst)\n'
        + "{\n"
        + align+"BUILD_ASSERT_DECL(FLOW_WC_SEQ == 43);\n"
        + align+"\n"
        + align+"const struct pkt_metadata *md = &packet->md;\n"
        + align+"void *data = dp_packet_data(packet);\n"
        + align+"size_t size = dp_packet_size(packet);\n"
        + align+"ovs_be32 packet_type = packet->packet_type;\n"
        + align+'uint64_t *values = miniflow_values(dst);\n'
        + align +
        "struct mf_ctx mf = { FLOWMAP_EMPTY_INITIALIZER, values,values + FLOW_U64S_mof };\n"
        + align+"const char *frame;\n"
        + align+'uint8_t nw_frag, nw_tos, nw_ttl, nw_proto;\n'
        + align+'uint8_t *ct_nw_proto_p = NULL;\n'
        + align+'ovs_be16 ct_tp_src = 0, ct_tp_dst = 0;\n'
        + align + ret_srv6_flags(srv6num) + '\n'
        + align + 'packet->flag = 0;\n'
        + '\n'
        + pasermetadata + '\n'
        + align + '''// uint8_t *bi = mf.map.bits;
    // VLOG_INFO("bits[0] after metadata push:%x %x %x %x %x %x %x %x",bi[7],bi[6],bi[5],bi[4],bi[3],bi[2],bi[1],bi[0]);

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

    //VLOG_INFO("flow_modify");

    //flow_modify((uint8_t *)data);
    
    miniflow_push_macs(mf, dl_dst, data);\n'''
        + ofs_inits + '\n'
    )

##################################################################

def Ttype_get(dHead, name, Ttype, Ttype_size):
    # print('Ttypesize:',Ttype_size)
    # print('Ttype:',Ttype)
    # print('name:',name)
    dSizeToType = {8: 'uint8_t', 16: 'ovs_be16',
                   32: 'ovs_be32', 64: 'ovs_be64'}
    file_path = './miniflow_extract.c'
    
    typeflag = 0
    for key, val in dHead.items():
        # print(key)
        # print(val)
        for key_, val_ in val.items():
            if('type' in key_):
                if(val.get('Type') != None):
                    Ttype[key_] = val.get('Type')
                Ttype_size[key_] = dSizeToType[(val.get(key_))['size']]


def defineinit(Ttype):
    with open('./cfile/miniflow_extract.c', 'w', encoding='utf-8') as fo:
        for key, value in Ttype.items():
            if (value != None):
                for key_, value_ in value.items():
                    #print(key_, value_)
                    if(len(value_) == 4):
                        fo.write('#define ' + key_ + ' ' + value_ + '\n')
                    elif(len(value_) == 6):
                        fo.write('#define ' + key_ + ' ' + "ntohs(" + value_ + ')\n')
                    elif(len(value_) == 10):
                        fo.write('#define ' + key_ + ' ' + "ntohl(" + value_ + ')\n')
                    elif(len(value_) == 18):
                        fo.write('#define ' + key_ + ' ' + "ntohll(" + value_ + ')\n')


def iteration(typename, dtype, Ttype, Ttype_size, fo, cnt):
    # print(typename)
    # print(dtype)
    # print(Ttype)
    # print(Ttype_size)
    # print('\n\n')
    if (Ttype_size.get(typename) != None):
        fo.write(get_parser(typename, Ttype_size[typename], cnt))
    else:
        fo.write(get_parser_without_type(typename, cnt))
    if (dtype != None):
        i = 0
        for key, val in dtype:
            if (i == 0):
                fo.write(get_if(typename, key, cnt) + align*cnt + '{\n')
                typename_ = key[1-(len(key)-len(typename)):]+'_type'
                # print(typename_ + '------------')
                if (Ttype.get(typename_) != None):
                    dtype_ = Ttype[typename_].items()
                else:
                    dtype_ = None
                # if(Ttype_size.get(typename_)!=None):
                iteration(typename_, dtype_, Ttype, Ttype_size, fo, cnt+1)
                fo.write(cnt*align+'}\n')
            else:
                if (i < len(Ttype[typename])):
                    fo.write(get_elif(typename, key, cnt) + align*cnt + '{\n')
                    typename_ = key[1-(len(key)-len(typename)):]+'_type'
                    if (Ttype.get(typename_) != None):
                        dtype_ = Ttype[typename_].items()
                    else:
                        dtype_ = None
                    # if(Ttype_size.get(typename_)!=None):
                    iteration(typename_, dtype_, Ttype, Ttype_size, fo, cnt+1)
                    fo.write(cnt*align+'}\n')
            i = i+1


def create_extract(Ttype, Ttype_size, start, srv6num):
    fo = open('./cfile/miniflow_extract.c', 'a', encoding='utf-8')
    defineinit(Ttype)
    extract_init(fo, srv6num)
    # print(start)
    # print(Ttype[start])
    iteration(start, Ttype[start].items(), Ttype, Ttype_size, fo, 1)
    fo.write('''    if(g_num_vlans == 1){
        packet->flag |= VLAN1;
    }
    if(g_num_vlans == 2){
        packet->flag |= VLAN1;
        packet->flag |= VLAN2;
    }
    packet->flag |= MAC;

    dst->map = mf.map;\n}''')


# import yaml
# import json
# import createflow
# import createparser
# align = "    "
# pasermetadata = '/* Metadata. */ \n\
#     if (flow_tnl_dst_is_set(&md->tunnel)) {\n\
#         miniflow_push_words(mf, tunnel, &md->tunnel,\n\
#                             offsetof(struct flow_tnl, metadata) /\n\
#                             sizeof(uint64_t));\n\
# \n\
#         if (!(md->tunnel.flags & FLOW_TNL_F_UDPIF)) {\n\
#             if (md->tunnel.metadata.present.map) {\n\
#                 miniflow_push_words(mf, tunnel.metadata, &md->tunnel.metadata,\n\
#                                     sizeof md->tunnel.metadata /\n\
#                                     sizeof(uint64_t));\n\
#             }\n\
#         } else {\n\
#             if (md->tunnel.metadata.present.len) {\n\
#                 miniflow_push_words(mf, tunnel.metadata.present,\n\
#                                     &md->tunnel.metadata.present, 1);\n\
#                 miniflow_push_words(mf, tunnel.metadata.opts.gnv,\n\
#                                     md->tunnel.metadata.opts.gnv,\n\
#                                     DIV_ROUND_UP(md->tunnel.metadata.present.len,\n\
#                                                  sizeof(uint64_t)));\n\
#             }\n\
#         }\n\
#     }\n\
#     if (md->skb_priority || md->pkt_mark) {\n\
#         miniflow_push_uint32(mf, skb_priority, md->skb_priority);\n\
#         miniflow_push_uint32(mf, pkt_mark, md->pkt_mark);\n\
#     }\n\
#     miniflow_push_uint32(mf, dp_hash, md->dp_hash);\n\
#     miniflow_push_uint32(mf, in_port, odp_to_u32(md->in_port.odp_port));\n\
#     if (md->ct_state) {\n\
#         // VLOG_INFO("ct_state exsists");\n\
#         miniflow_push_uint32(mf, recirc_id, md->recirc_id);\n\
#         miniflow_push_uint8(mf, ct_state, md->ct_state);\n\
#         ct_nw_proto_p = miniflow_pointer(mf, ct_nw_proto);\n\
#         miniflow_push_uint8(mf, ct_nw_proto, 0);\n\
#         miniflow_push_uint16(mf, ct_zone, md->ct_zone);\n\
#         miniflow_push_uint32(mf, ct_mark, md->ct_mark);\n\
#         miniflow_push_be32(mf, packet_type, packet_type);\n\
#         if (!ovs_u128_is_zero(md->ct_label)) {\n\
#             miniflow_push_words(mf, ct_label, &md->ct_label,\n\
#                                 sizeof md->ct_label / sizeof(uint64_t));\n\
#         }\n\
#     } else {\n\
#         // VLOG_INFO("ct_state not exsists");\n\
#         if (md->recirc_id) {\n\
#             // VLOG_INFO("recirc_id exsists");\n\
#             miniflow_push_uint32(mf, recirc_id, md->recirc_id);\n\
#             miniflow_pad_to_64(mf, recirc_id);\n\
#         }\n\
#         miniflow_pad_from_64(mf, packet_type);\n\
#         miniflow_push_be32(mf, packet_type, packet_type);\n\
#     }\n\
# '

# def get_if(tp_name, value, cnt):
#     return align*cnt + 'if (' + tp_name + ' == ' + value + ')\n'

# def get_elif(tp_name, value, cnt):
#     return align*cnt + 'else if (' + tp_name + ' == ' + value + ')\n'

# def get_parser(name, size, cnt):
#     name_ = name[0:len(name)-len('_type')]
#     return align*cnt + size + ' ' + name + ' = ' + 'parser_' + name_ + ('(&data, &size, &mf);\n')


# def extract_init(fo):
#         fo.write(
#             '\n\nvoid miniflow_extract_mof(void *packet,struct miniflow *dst)\n'
#             + "{\n"
#             + align+"BUILD_ASSERT_DECL(FLOW_WC_SEQ == 42);\n"
#             + align+"\n"
#             + align+"const struct pkt_metadata *md = &packet->md;\n"
#             + align+"void *data = dp_packet_data(packet);\n"
#             + align+"size_t size = dp_packet_size(packet);\n"
#             + align+"ovs_be32 packet_type = packet->packet_type;\n"
#             + align+'uint64_t *values = miniflow_values(dst);\n'
#             + align+"struct mf_ctx mf = { FLOWMAP_EMPTY_INITIALIZER, values,values + FLOW_U64S_mof };\n"
#             + align+"const char *frame;\n"
#             + align+'ovs_be16 dl_type = OVS_BE16_MAX;\n'
#             + align+'uint8_t nw_frag, nw_tos, nw_ttl, nw_proto;\n'
#             + align+'uint8_t *ct_nw_proto_p = NULL;\n'
#             + align+'ovs_be16 ct_tp_src = 0, ct_tp_dst = 0;\n'
#             + '\n'
#             + pasermetadata + '\n'
#             + align+'frame = data;\n'
#             + align+'dp_packet_reset_offsets(packet);\n\n'
#         )


# def Ttype_get(dHead,name,Ttype,Ttype_size):
#     dSizeToType = {8: 'uint8_t', 16: 'ovs_be16', 32: 'ovs_be32'}
#     file_path = './miniflow_extract.c'

#     for key, val in dHead.items():
#         dHeadVal = val
#         #print(dHeadVal)
#         Ttype[name+'_type'] = dHeadVal.get('Type')
#         Ttype_size[name+'_type'] = dSizeToType[(dHeadVal.get(name+'_type'))['size']]


# def defineinit(Ttype):
#     with open('./cfile/miniflow_extract.c', 'w', encoding='utf-8') as fo:
#         for key,value in Ttype.items():
#             if (value != None):
#                 for key_, value_ in value.items():
#                     fo.write('#define ' + key_ + ' ' + value_ +'\n')


# def iteration (typename, dtype, Ttype,Ttype_size,fo,cnt):
#     fo.write(get_parser(typename,Ttype_size[typename],cnt))
#     if (dtype != None):
#         i=0
#         for key, val in dtype:
#             if(i==0):
#                 fo.write(get_if(typename,key,cnt) + align*cnt + '{\n')
#                 typename_ = key[1-(len(key)-len(typename)):]+'_type'
#                 if (Ttype[typename_]!=None):
#                     dtype_ = Ttype[typename_].items()
#                 else:
#                     dtype_ = Ttype[typename_]
#                 iteration (typename_, dtype_, Ttype, Ttype_size,fo,cnt+1)
#                 fo.write(cnt*align+'}\n')
#             else:
#                 if(i<len(Ttype[typename])):
#                     fo.write(get_elif('eth_type',key,cnt) + align*cnt + '{\n')
#                     typename_ = key[1-(len(key)-len(typename)):]+'_type'
#                     if (Ttype[typename_]!=None):
#                         dtype_ = Ttype[typename_].items()
#                     else:
#                         dtype_ = Ttype[typename_]
#                     iteration (typename_, dtype_, Ttype, Ttype_size,fo,cnt+1)
#                     fo.write(cnt*align+'}\n')
#             i=i+1


# def create_extract(Ttype,Ttype_size,start):
#     fo = open('./cfile/miniflow_extract.c', 'a', encoding='utf-8')
#     defineinit(Ttype)
#     extract_init(fo)
#     iteration (start, Ttype[start].items(),Ttype,Ttype_size,fo,1)
#     fo.write('\n    dst->map = mf.map;\n}')
