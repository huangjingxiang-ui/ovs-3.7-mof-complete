import yaml
import json

outputpath1 = './cfile/match.c'
fo1 = open(outputpath1, 'w', encoding='utf-8')

str_start = '''void
match_format_mof(const struct match *match,
             const struct ofputil_port_map *port_map,
             struct ds *s, int priority)
{
    const struct flow_wildcards *wc = &match->wc;
    size_t start_len = s->length;
    const struct flow *f = &match->flow;
    bool skip_type = false;
    bool skip_proto = false;
    ovs_be16 dl_type = f->dl_type;
    bool is_megaflow = false;
    int i;
    const struct flow_mof *flow_mof = &match->flow;
    const struct flow_mof *wc_mof = &match->wc.masks;
    int bytes, bits;
    uint8_t *value, *mask;

    if (priority != OFP_DEFAULT_PRIORITY) {
        ds_put_format(s, "%spriority=%s%d,",
                      colors.special, colors.end, priority);
    }

    if (wc->masks.in_port.ofp_port) {
        ds_put_format(s, "%sin_port=%s", colors.param, colors.end);
        ofputil_format_port(f->in_port.ofp_port, port_map, s);
        ds_put_char(s, ',');
    }
'''

str_end = '''
    if (s->length > start_len) {
        ds_chomp(s, ',');
    }
}\n\n'''

str_mid = '''   if (*(uint8_t*)&wc_mof->FIELDS) {
        bytes = sizeof(flow_mof->FIELDS);
        ds_put_format(s, "%sFIELDS=%s", colors.param, colors.end);
        ds_put_hex(s, &flow_mof->FIELDS, bytes);
        bits = count_field_1bit(&wc_mof->FIELDS, bytes);
        if (bits != bytes * 8) {
            ds_put_format(s, "/%d", bits);
        }
        ds_put_char(s, ',');
    }
'''

def create_match_format_mof_start():
    fo1.write(str_start)

def create_match_format_mof_mid(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                fo1.write(str_mid.replace('FIELDS', key_))
        
def create_match_format_mof_end():
    fo1.write(str_end)


