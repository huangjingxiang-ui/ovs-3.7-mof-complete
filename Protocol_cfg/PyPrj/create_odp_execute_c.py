outputpath1 = './cfile/odp-execute/case_SEG_ROUTING.c'
fo1 = open(outputpath1, 'w', encoding='utf-8')

str_case_seg_routing = '''\n        case OVS_ACTION_ATTR_SEG_ROUTING: {
            DP_PACKET_BATCH_FOR_EACH (i, packet, batch) {
                seg_routing(packet);
            }
            break;
        }\n'''
        
def create_case_seg_routing(srv6_exsist):
    if(srv6_exsist == True):
        fo1.write(str_case_seg_routing)
    else:
        fo1.write('\n')
        
        