outputpath1 = './cfile/ofproto-dpif-xlate/xlate-offset.c'
fo1 = open(outputpath1, 'w', encoding='utf-8')

outputpath2 = './cfile/ofproto-dpif-xlate/xlate-srv6.c'
fo2 = open(outputpath2, 'w', encoding='utf-8')

outputpath3 = './cfile/flowmap_set_mof.c'
fo3 = open(outputpath3, 'w', encoding='utf-8')

first_fields = []
start = '\n\nsize_t flow_mof_protocol_ofs[] = {\n'
end = '};\n'

str_srv6_true = '''\n        case OFPACT_SEG_ROUTING: {
            nl_msg_put_unspec(ctx->odp_actions, OVS_ACTION_ATTR_SEG_ROUTING, 
                                NULL, 0);
            xlate_set_flow_value_srv6(&ctx->xin->flow);
            break;
        }
        }

        /* Check if need to store this and the remaining actions for later
         * execution. */
        if (!ctx->error && ctx->exit && ctx_first_frozen_action(ctx)) {
            freeze_unroll_actions(a, ofpact_end(ofpacts, ofpacts_len), ctx);
            break;
        }
    }
}

void 
xlate_set_flow_value_srv6(uint8_t *flow_mof)
{
    enum pmap flag;
    size_t offset_srv6, offset_ipv6;
    uint8_t *segment_left, *hdr_len;
    int index_srv6 = -1, index_ipv6 = -1;

    for (flag = SRV6_1; flag > 0;) {
        index_srv6++;
        flag >>= 1;
    }
    for (flag = IPV6_E; flag > 0;) {
        index_ipv6++;
        flag >>= 1;
    }

    offset_srv6 = flow_mof_protocol_ofs[index_srv6];
    offset_ipv6 = flow_mof_protocol_ofs[index_ipv6];

    segment_left = flow_mof+offset_srv6+3;
    hdr_len = flow_mof+offset_srv6+1;
    *segment_left -= 1;
    if (*segment_left == 255) {
        memset(flow_mof+offset_srv6, 0, *hdr_len);
        xlate_set_flow_zero(flow_mof, IPV4_E);
        xlate_set_flow_zero(flow_mof, IPV6_E);
        return;
    }
    memcpy(flow_mof+offset_ipv6+8+16, flow_mof+offset_srv6+8+16*(*segment_left), 16);
}

'''

str_srv6_false = '''        }

        /* Check if need to store this and the remaining actions for later
         * execution. */
        if (!ctx->error && ctx->exit && ctx_first_frozen_action(ctx)) {
            freeze_unroll_actions(a, ofpact_end(ofpacts, ofpacts_len), ctx);
            break;
        }
    }
}
'''

def add_first_field(field):
    first_fields.append(field)
    
def create_flow_mof_protocol_ofs(srv6_num):    
    fo1.write(start)
    for name in first_fields:
        if(name == 'srv6_type'):
            for i in range(srv6_num):
                fo1.write('    offsetof(struct flow_mof, ' + name + '),\n')
        else:
            fo1.write('    offsetof(struct flow_mof, ' + name + '),\n')
    fo1.write(end + '\n')
    
    
def create_xlate_srv6(srv6_exsist):
    if(srv6_exsist == True):
        fo2.write(str_srv6_true)
    else:
        fo2.write(str_srv6_false)


