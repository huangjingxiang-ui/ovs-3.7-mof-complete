import yaml
import json

outputpath1 = './cfile/packets/packets_c_1.c'
fo1 = open(outputpath1, 'w', encoding='utf-8')

outputpath2 = './cfile/packets/packets_c_2.c'
fo2 = open(outputpath2, 'w', encoding='utf-8')


# ---------------------------------------------#
phlens_start = '\n\nsize_t phlens[] = {\n'
phlens_end = '};\n'
rx_name = []
crc_offset = {}
crc_len = {}

crc_ofs_start = 'size_t crc_ofs[] = {\n'
crc_ofs_end = '};\n'

crc_len_start = 'size_t crc_len[] = {\n'
crc_len_end = '};\n'
# --------------------------------------------#
str_srv6_true1 = '''
void 
seg_routing(struct dp_packet *packet) 
{   
    size_t offset_ipv6, offset_srv6;
    uint8_t *header, *segment_left, *hdr_len;
    struct srv6_1 *srv6;

    if (packet->flag | IPV6_E) {
'''

str_srv6_true2 = '''
            return;
    }

    get_offset_index(packet, IPV6_E, &offset_ipv6, NULL);
    get_offset_index(packet, SRV6_1, &offset_srv6, NULL);

    header = (uint8_t*)dp_packet_data(packet) + offset_srv6;
    hdr_len = header + 1;
    segment_left = header + 3;

    *segment_left -= 1; 
    if (*segment_left == 255) {
        pop_protocol(packet, offset_srv6, *hdr_len);
        del_protocol(packet, IPV4_E);
        del_protocol(packet, IPV6_E);
        return;
    }

    // *segment_left = n, move segment_list[n] to ipv6 dst
    memcpy(dp_packet_data(packet)+offset_ipv6+8+16, header+8+16*(*segment_left), 16);
      
}

'''


def mid_out(srv6num):
    str_srv6_mid = '''        if (!(packet->flag '''
    for i in range(srv6num):
        str_srv6_mid += "| SRV6_" + str(i+1) + " "
    str_srv6_mid += ")) "
    return str_srv6_mid
    


def addname(name):
    rx_name.append(name)


def crc_handler(Li_result):
    offset = {}
    i = 0
    val_save = ''
    for key, val in Li_result.items():
        crc_name = key + '_crc'
        crc_offset[crc_name] = 0
        crc_len[crc_name] = 0
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash'):
                if i == 0:
                    offset[key_] = 0
                else:
                    offset[key_] = offset[key_pre] + val_pre['size']
                key_pre = key_
                val_pre = val_
                i += 1
            if ('crc' in key_):
                crc_len[key_] = int(val_['size']/8)
    for key, val in offset.items():
        if 'crc' in key:
            crc_offset[key] = val


def create_packets_c_1(srv6num):
    fo1.write(phlens_start)
    for name in rx_name:
        if (name != 'srv6'):
            fo1.write('  sizeof(struct ' + name + '),\n')
        else:
            for i in range(srv6num):
                fo1.write('  sizeof(struct ' + name + '_' + str(i+1) + '),\n')

    fo1.write(phlens_end + '\n')

    # print(crc_offset)
    for key, val in crc_offset.items():
        crc_offset[key] = int(val/8)
    fo1.write(crc_ofs_start)
    for key, val in crc_offset.items():
        #print(key, val)
        fo1.write('          '+str(val) + ',\n')
    fo1.write(crc_ofs_end + '\n')

    fo1.write(crc_len_start)
    for key, val in crc_len.items():
        fo1.write('          '+str(val) + ',\n')
    fo1.write(crc_len_end + '\n')


def create_packets_c_2(srv6num):
    if (srv6num != 0):
        fo2.write(str_srv6_true1)
        fo2.write(mid_out(srv6num))
        fo2.write(str_srv6_true2)
    else:
        fo2.write('\n')


# ----------------------------------------------------------------------------------------- #
# def create_phlens():
#     fo1.write(phlens_start)
#     for name in rx_name:
#         fo1.write('  sizeof(struct ' + name + '),\n')
#     fo1.write(phlens_end)


# def create_crc_ofs():
#     for key, val in crc_offset.items():
#         crc_offset[key] = int(val/8)
#     fo1.write(crc_ofs_start)
#     for key, val in crc_offset.items():
#         fo1.write('          '+str(val) + ',\n')
#     fo1.write(crc_ofs_end)


# def create_crc_len():
#     # for key, val in crc_len.items():
#     #     crc_len[key] = int(val/8)
#     fo1.write(crc_len_start)
#     for key, val in crc_len.items():
#         fo1.write('          '+str(val) + ',\n')
#     fo1.write(crc_len_end)
