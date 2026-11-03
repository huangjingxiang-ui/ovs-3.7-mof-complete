import yaml

#meta-flow.c

def vSetInit():
    file_path = './cfile/meta-flow.c'
    func_start = "char * \n\
mf_parse_set_mof(const char *name, const char *val, struct match *match, \n\
        const struct ofputil_port_map *port_map, \n\
        union mf_value *value, union mf_value *mask) \n\
{ \n\
    struct flow_mof *flow_mof, *wc_mof; \n\
    flow_mof = (struct flow_mof*)(&match->flow); \n\
    wc_mof = (struct flow_mof*)(&match->wc); \n\
    int bytes; \n\
    uint8_t *value_t, *mask_t; \n\
    char *error; \n\
    if (!strcmp(name, \"in_port\")) { \n\
    error = mf_from_ofp_port_string_mof(val, port_map, &value->be16, &mask->be16); \n\
    match_set_in_port(match, u16_to_ofp(ntohs(value->be16)));\n"
    with open(file_path, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(func_start)
    # print(func_start)



def vCreateSet(Li_result,srv6num):
    for _, val in Li_result.items():
        for k, _ in val.items():
            if k == "Hash" or k == "Type":
                continue
            file_path = './cfile/meta-flow.c'
            with open(file_path, mode='a', encoding='utf-8') as file_obj:
                func = "\
    } else if (!strcmp(name, \"special_field\")) { \n\
        bytes = sizeof(flow_mof->special_field); \n\
        value_t = (uint8_t*)(&flow_mof->special_field); \n\
        mask_t = (uint8_t*)(&wc_mof->special_field); \n\
        error = mf_from_integer_string_mof(bytes, val, (uint8_t*)value, (uint8_t*)mask); \n\
        mf_set_mof(bytes, (uint8_t*)value, value_t, (uint8_t*)mask, mask_t);\n"
                if k == 'srv6_segmentlist':
                    for i in range(srv6num):
                        k_ = "srv6_segmentlist" +  str(i+1)
                        func_ = func.replace("special_field",k_)
                        file_obj.write(func_)    
                else:
                    func = func.replace("special_field", k)
                    file_obj.write(func)
                # print(func)
            

def vSetFinish():
    func_end = "\
    } else { \n\
        error = xasprintf(\"%s: unknown field\", name); \n\
    } \n\
    return error; \n\
}\n"
    file_path = './cfile/meta-flow.c'
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(func_end)
    # print(func_end)
