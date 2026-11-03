dEth = {'Ethernet': {'dstAddr': {'size': 48}, 'srcAddr': {'size': 48}, 'etherType': {'size': 16}}}


EthFlow = 'struct eth_addr dl_dst; \n \
    struct eth_addr dl_src;\n \
    ovs_be16 dl_type;\n \
    uint8_t pad1[2];\n'

Vlan_Mpls = 'union flow_vlan_hdr vlans[FLOW_MAX_VLAN_HEADERS]; \n\
     ovs_be32 mpls_lse[ROUND_UP(FLOW_MAX_MPLS_LABELS, 2)];\n'

Ipv4Flow = 'ovs_be32 nw_src;\n \
    ovs_be32 nw_dst;\n \
    ovs_be32 ct_nw_src;\n\
    ovs_be32 ct_nw_dst;\n'

Ipv6Flow = 'struct in6_addr ipv6_src;\n\
    struct in6_addr ipv6_dst;\n\
    struct in6_addr ct_ipv6_src;\n\
    struct in6_addr ct_ipv6_dst;\n\
    ovs_be32 ipv6_label;\n\
    struct in6_addr nd_target;\n'

Ipv4HelpFlow = 'uint8_t nw_frag;\n\
    uint8_t nw_tos;\n\
    uint8_t nw_ttl;\n\
    uint8_t nw_proto;\n\
    ovs_be32 pad2;\n'


TcpUdpStcpFlow = 'ovs_be16 tp_src;\n\
    ovs_be16 tp_dst;\n\
    ovs_be16 ct_tp_src;\n\
    ovs_be16 ct_tp_dst;\n\
    ovs_be32 igmp_group_ip4;\n\
    ovs_be32 pad3;\n'

# 填充计数。bits初值为metadata比特�?,flow_len为原flow比特总长�?
# OVS 3.7 layout (verified against the final OVS 3.7-MOF build):
# the generated protocol area starts at byte 536 and the native flow fields
# end at byte 736.
bits, flow_len = 536*8, 736*8
enumpmapcnt = 0

# 存在协议对应的字符串
dExistName = {'eth': EthFlow, 'ipv4': Ipv4Flow, 'ipv6': Ipv6Flow, 'tcp': TcpUdpStcpFlow, 'udp': TcpUdpStcpFlow,
              'icmp': TcpUdpStcpFlow}
dExistName_len = {'eth': 128, 'ipv4': 192, 'ipv6': 576, 'tcp': 128, 'udp': 128, 'icmp': 128}

# 长度超出的创建结构体
def creatheader(num, name):
    sHeaderStruct ='struct XXX{\n\
        union {\n\
            uint8_t eb[ucsize];\n\
        };\n\
    };\n'

    #mod
    mod = 0
    if(num > 1280):
        mod = num % 128
        num = 128
        
    num = (num / 8)
    num = int(num)
    snum = str(num)
    numsize = len(snum)

    num16 = num / 2
    num16 = int(num16)
    snum16 = str(num16)
    num16size = len(snum16)

    stname = 'st' + str(name)
    sname = sHeaderStruct.replace('XXX',stname)
    sname = sname.replace('ucsize',snum)

    #sHeaderStruct = str(sHeaderStruct)
    file_path = './cfile/header.c'
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(sname)
    #print(sname)
    if(mod != 0):
        return 'st' + str(name),mod
    return 'st' + str(name),0

def vCreateEnumPmap(name,enumpmapcnt):
    enumpmapstr = '    ' + name.upper() + ' = 1 << ' + str(enumpmapcnt) + ',\n'
    file_path = './cfile/enumpmap.c'
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
        if(name != "srv6"):
            file_obj.write(enumpmapstr)
        else :
            return -1

def vCreateUnionProto(name):
    file_path = './cfile/unionprotols.c'
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
        if name == 'srv6':
            file_obj.write('')
        else:
            file_obj.write("    struct " + name +' ' + name +';\n')
   
    
     
def vCreateSmallStInit(name):
    file_path = './cfile/smallstruct.c'
    if name == "srv6":
        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            file_obj.write("\n")
    else:
        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            file_obj.write("struct " + name + "{"+'\n')



    

def vCreateSmallSt(name,size,srv6_flag):
    # print(srv6_flag)
    dSizeToType = {}#{8: 'uint8_t', 16: 'ovs_be16', 32: 'ovs_be32', 64: 'ovs_be64'}
    sSrv6 = 'struct srv6_xx{\n\
    uint8_t srv6_type;\n\
    uint8_t srv6_hdr_ext_len;\n\
    uint8_t srv6_routing_Type;\n\
    uint8_t srv6_segments_left;\n\
    uint8_t srv6_last_enty;\n\
    uint8_t srv6_flags;\n\
    ovs_be16 srv6_tag;\n'
    
    segement_list = '    struct stsrv6_segmentlist srv6_segmentlist[i];\n'

    file_path = './cfile/smallstruct.c'
    if(srv6_flag == 0):
        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            if(size in dSizeToType.keys()):
                file_obj.write('    ' + dSizeToType[size] + ' ' + name +';\n')
            # elif size > 128:
            #     with open(file_path, mode='a', encoding='utf-8') as file_obj:
            #         size = str(size)
            #         file_obj.write('     ' + 'ovs_be128 srh_segmentlist[' + size[-1] + '];\n')
            else:
                mod = 0
                if(size > 1280):
                    mod = size % 128
                    size = 128
                    
                if(mod != 0):
                    mod = int(mod)
                    # mod = str(mod)
                    # file_obj.write('    struct st' + name +' ' + name + '[' + mod + '];\n')
                else:
                    cnt = size /8
                    cnt = int(cnt)
                    file_obj.write('    uint8_t ' + name + '[' + str(cnt) + '];''\n')
    else:
        mod = 0
        if(size > 128):
            mod = size % 128
            size = 128
            
        if(mod != 0):
            mod = int(mod)
        for i in range(mod):
            i = i + 1
            inti = i
            i = str(i)
            with open('./cfile/smallstruct.c', mode='a', encoding='utf-8') as file_obj:
                # file_obj.write("str1")
                str0 = sSrv6.replace('xx',i)
                file_obj.write(str0)
                #str1 = str0.replace('[i]',i)
                for j in range(inti):
                    j = j + 1
                    j = str(j)
                    seg = segement_list.replace('[i]',j)
                    file_obj.write(seg)
                file_obj.write('};\n\n')
            
                

def vCreateSmallStEnd(name):
    file_path = './cfile/smallstruct.c'
    if name != 'srv6':
        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            file_obj.write('};\n\n')
#main
def vCreateFlow(dHead, name):
    global bits
    global enumpmapcnt
    srv6num = 0
    #已存在协�?
    if name is not None:
        file_path = './cfile/flow.h'
        stwrite = dExistName[name]
        bits += int(dExistName_len[name])
        #ip需要特殊处理一�?
        if name == 'ipv4':
            with open(file_path, mode='a', encoding='utf-8') as file_obj:
                file_obj.write('     ' + stwrite + '     ' + Ipv4HelpFlow)
        else:
            with open(file_path, mode='a', encoding='utf-8') as file_obj:
                file_obj.write('     ' + stwrite)
    #新协�?
    else:
        dSizeToType = {8: 'uint8_t', 16: 'ovs_be16', 32: 'ovs_be32',64: 'ovs_be64'}
        dSizeToTypeno = {}
        padsize = 0
        dHeadname = []
        file_path = './cfile/flow.h'
        srv6_flag = None
        for key, val in dHead.items():
            srv6_flag = key
            vCreateSmallStInit(key)
            vCreateUnionProto(key)
            ret = vCreateEnumPmap(key,enumpmapcnt)
            if ret != -1:
                enumpmapcnt += 1                
            dHeadVal = val
            dHeadname.append(key)
			
        if('Type' in val.keys()):
            val.pop('Type')
        if('Hash' in val.keys()):
            val.pop('Hash')
        
        for key, val in dHeadVal.items():
            for key1, size in val.items():
                if srv6_flag == 'srv6':
                    vCreateSmallSt(key,size,1)
                else :
                    vCreateSmallSt(key,size,0)
                if srv6_flag != 'srv6':
                    padsize += size
                else :
                    srv6num = size % 128
                    if key=='srv6_segmentlist':
                        bits += srv6num * 128
                    else:
                        bits+=size
                    # print('bits: ',bits)
                    if key=='srv6_segmentlist':
                        file_path_enumpmap = './cfile/enumpmap.c'
                        with open(file_path_enumpmap, mode='a', encoding='utf-8') as file_obj:
                            for i in range(srv6num):
                            
                                file_obj.write('    SRV6_'  + str(i + 1) + ' = 1 << ' + str(enumpmapcnt) + ',\n')
                                print('    SRV6_'  + str(i + 1) + ' = 1 << ' + str(enumpmapcnt) + ',\n')
                                enumpmapcnt = enumpmapcnt + 1
                if size in dSizeToTypeno.keys():
                    with open(file_path, mode='a', encoding='utf-8') as file_obj:
                        file_obj.write('     ' + dSizeToTypeno[size] + '  ' + key + ';\n')
       
                else:
                    sname,mod = creatheader(size,key)
                    # if srv6_flag != 'srv6'
                    #     mod = 0
                    swrite = 'struct  ' + sname +'  ' + key
                    with open(file_path, mode='a', encoding='utf-8') as file_obj:
                        if(mod != 0):
                            mod = int(mod)
                            for i in range(mod):
                                i += 1
                                i = str(i)
                                file_obj.write('     ' + swrite + i + ';\n')
                        else:
                            cnt = size/8
                            cnt = int(cnt)
                            file_obj.write('     ' +'uint8_t ' + key + '[' + str(cnt) + '];' + '\n')
        
        vCreateSmallStEnd(srv6_flag)
        # 填充�?64bit
        bits += padsize
        if ((padsize % 64) and (dHeadname[-1] != 'vlan1') and (dHeadname[-1] != 'vlan2')):
            inx = 64 - padsize % 64
            bits += inx
            if inx in dSizeToType:
                with open(file_path, mode='a', encoding='utf-8') as file_obj:
                    file_obj.write('     ' + dSizeToType[inx] + ' pad_' + dHeadname[-1] + ';\n')
            else:
                with open(file_path, mode='a', encoding='utf-8') as file_obj:
                    inx = inx // 8

                    file_obj.write('     ' + dSizeToType[8] + ' pad_' + dHeadname[-1] + '[' + str(inx) + ']' + ';\n')
        # print(dHead, bits)            
        # for key, val in dHead.items():                 
        #     if key == 'mac':
        #         with open(file_path, mode='a', encoding='utf-8') as file_obj:
        #                 file_obj.write('\n     ' + Vlan_Mpls + '\n')

        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            file_obj.write('\n')
        return padsize ,srv6num




def vFlowInit():
    sFirstLine = 'struct flow_mof{ \n \
    uint8_t pad_start[536];\n'

    file_path = './cfile/flow.h'
    with open(file_path, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(sFirstLine)

    with open('./cfile/header.c', mode='w', encoding='utf-8') as test:
        test.write(' ')


def vOriginFlowInit():
    file_path = './cfile/ori_flow_pad.c'
    with open(file_path, mode='w', encoding='utf-8') as file_obj:
        file_obj.write('')


def vCreateSmallStructInit():
    file_path = './cfile/smallstruct.c'
    with open(file_path, mode='w+', encoding='utf-8') as file_obj:
        file_obj.write('\n')
        
def vCreateUnionProtolsInit():
    file_path = './cfile/unionprotols.c'
    with open(file_path, mode='w', encoding='utf-8') as file_obj:
        file_obj.write('union protocols {\n')      


def vCreateUnionProtolsEnd(srv6_num):
    file_path = './cfile/unionprotols.c'
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
        for i in range(srv6_num):
            file_obj.write("    struct " + "srv6" +'_' + str(i + 1) + " srv6" +'_' + str(i + 1) +';\n')
        file_obj.write('};\n')     


def vFlowEnd():
    file_path = './cfile/flow.h'
    if (flow_len - bits) > 0:
        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            # 填充flow_mof长度与flow一�?
            file_obj.write('     ' + 'ovs_be64' + ' pad_end' + '[' + str((flow_len - bits) // 64) + ']' + ';\n')
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
        file_obj.write('};\n')
    with open('./cfile/header.c', mode='a', encoding='utf-8') as test:
        test.write('\n\n\n')
    return flow_len


def vOriginFlowEnd():
    # print(bits)
    file_path = './cfile/ori_flow_pad.c'
    if (flow_len - bits) < 0:
        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            # 填充flow长度与flow_mof一�?
            file_obj.write('    ' + 'ovs_be64' + ' flow_pad_end' + '[' + str((bits - flow_len) // 64) + ']' + ';\n')
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
        file_obj.write('};\n\n')
    return bits


def vCreateEnumPmapInit():
    file_path = './cfile/enumpmap.c'
    with open(file_path, mode='w', encoding='utf-8') as file_obj:
        file_obj.write('enum pmap {\n')      


def vCreateEnumPmapEnd(srv6num):
    global enumpmapcnt
    #enumpmapcnt = enumpmapcnt - 1
    file_path = './cfile/enumpmap.c'
    with open(file_path, mode='a', encoding='utf-8') as file_obj:              
        file_obj.write('};\n')     
