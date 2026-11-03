outputpath1 = './cfile/flowmap_set_mof.c'
fo1 = open(outputpath1, 'w', encoding='utf-8')
srv6_type_written = False


def create_flowmap_set(Li_result):
    global srv6_type_written
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_:
                # All SRv6 variants share flow_mof.srv6_type in the verified
                # OVS 3.7 layout, so the map must contain it only once.
                if key_ == 'srv6_type':
                    if srv6_type_written:
                        continue
                    srv6_type_written = True
                fo1.write('\tFLOWMAP_SET_MOF(map, ' + key_ + ');\n')
