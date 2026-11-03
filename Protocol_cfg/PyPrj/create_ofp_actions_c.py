import yaml
import json

outputpath = './cfile/ofp-actions.c'
fo = open(outputpath, 'w', encoding='utf-8')
protocol_name = []


# ----------------------------parse_protocol---------------------------------


def parse_protocol_init(pro):
    pro_up = pro.upper()
    fo.write('\n\nchar * \n\
parse_protocol_%s(const char *s, const struct ofpact_parse_params *pp)\n\
{\n\
    char *key, *val;\n\
    struct %s %s;\n\
    struct ofpact_add_protocol *oap = ofpact_put_ADD_PROTOCOL(pp->ofpacts);\n\
    int bytes;\n\
    union mf_value value;\n\
    uint8_t *value_t;\n\
\n\
    oap->flag = %s;\n\
\n\
    memset(&%s, 0, sizeof(%s));\n\
    while(ofputil_parse_key_value(&s, &key, &val)) {\n        '
             % (pro, pro, pro, pro_up, pro, pro))


def parse_protocol_branch(pro, field, mod):
    if (mod == 0):
        fo.write('if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(%s.%s);\n\
            value_t = (uint8_t*)(&%s.%s);\n\
            mf_from_integer_string_mof(bytes, val, &value, NULL);\n\
            mf_set_mof(bytes, &value, value_t, NULL, NULL);\n\
        }' % (field, pro, field, pro, field))
    else:
        fo.write(' else if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(%s.%s);\n\
            value_t = (uint8_t*)(&%s.%s);\n\
            mf_from_integer_string_mof(bytes, val, &value, NULL);\n\
            mf_set_mof(bytes, &value, value_t, NULL, NULL);\n\
        }' % (field, pro, field, pro, field))


def parse_protocol_end(pro):
    fo.write(' else {\n\
            return xasprintf("unknown %s protocol field: ' % pro)
    fo.write('%s", key);\n\
        }\n\
    }\n')
    fo.write('    oap->protocol.%s = %s;\n\
    return NULL;\n\
}\n\n' % (pro, pro))

# ----------------------------------------------------------------------


def parse_ADD_PROTOCOL_init():
    fo.write('static char * OVS_WARN_UNUSED_RESULT\n\
parse_ADD_PROTOCOL(const char *arg, const struct ofpact_parse_params *pp)\n\
{\n\
    char *error = NULL;\n\
    \n\
\
')


def parse_ADD_PROTOCOL_branch(pro, mod):
    if (mod == 0):
        fo.write('    if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_protocol_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))
    else:
        fo.write('else if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_protocol_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))


def parse_ADD_PROTOCOL_end():
    fo.write('else {\n\
        return xasprintf("add unknown protocol");\n\
    }\n\
    return error;\n\
}\n\n')

# -----------------------------------------------------------------------


def parse_DEL_PROTOCOL_init():
    fo.write('static char * OVS_WARN_UNUSED_RESULT\n\
parse_DEL_PROTOCOL(const char *arg, const struct ofpact_parse_params *pp)\n\
{   \n\
    char *error = NULL;\n\
    struct ofpact_del_protocol *odp = ofpact_put_DEL_PROTOCOL(pp->ofpacts);\n\
\n\
')


def parse_DEL_PROTOCOL_branch(pro, mod):
    if (mod == 0):
        fo.write('    if (!strcmp(arg, "%s")) {\n\
        odp->flag = %s;\n\
    } ' % (pro, pro.upper()))
    else:
        fo.write('else if (!strcmp(arg, "%s")) {\n\
        odp->flag = %s;\n\
    } ' % (pro, pro.upper()))


def parse_DEL_PROTOCOL_end():
    fo.write('else {\n\
        return xasprintf("del unknown protocol");\n\
    }\n\
    return error;\n\
}\n\n')

# ---------------------------------------------------------------------------


def parse_protocol_out(Li_result):
    for key, val in Li_result.items():
        protocol_name.append(key)
        parse_protocol_init(key)
        i = 0
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                parse_protocol_branch(key, key_, i)
            i += 1
        parse_protocol_end(key)


def ADD_DEL_PROTOCOL_out():
    i = 0
    parse_ADD_PROTOCOL_init()
    for name in protocol_name:
        parse_ADD_PROTOCOL_branch(name, i)
        i += 1
    parse_ADD_PROTOCOL_end()

    i = 0
    parse_DEL_PROTOCOL_init()
    for name in protocol_name:
        parse_DEL_PROTOCOL_branch(name, i)
        i += 1
    parse_DEL_PROTOCOL_end()


# ---------------------------------parse fields-------------------------------------------

def parse_fields_init(pro):
    fo.write('char *\n\
parse_fields_%s(const char *s, const struct ofpact_parse_params *pp)\n\
{\n\
    char *key, *val;\n\
    struct %s %s;\n\
    struct %s masks;\n\
    struct ofpact_mod_fields *omf = ofpact_put_MOD_FIELDS(pp->ofpacts);\n\
    int bytes;\n\
    union mf_value value, mask;\n\
    uint8_t *value_t, *mask_t;\n\
\n\
    omf->flag = %s;\n\n\
    memset(&%s, 0, sizeof(%s));\n\
    memset(&masks, 0, sizeof(masks));\n\
    while(ofputil_parse_key_value(&s, &key, &val)) {\n        '
             % (pro, pro, pro, pro, pro.upper(), pro, pro))


def parse_fields_branch(pro, field, mod):
    if (mod == 0):
        fo.write('if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(%s.%s);\n\
            value_t = (uint8_t*)(&%s.%s);\n\
            mask_t = (uint8_t*)(&masks.%s);\n\
            mf_from_integer_string_mof(bytes, val, &value, &mask);\n\
            mf_set_mof(bytes, &value, value_t, &mask, mask_t);\n\
        } ' % (field, pro, field, pro, field, field))
    else:
        fo.write('else if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(%s.%s);\n\
            value_t = (uint8_t*)(&%s.%s);\n\
            mask_t = (uint8_t*)(&masks.%s);\n\
            mf_from_integer_string_mof(bytes, val, &value, &mask);\n\
            mf_set_mof(bytes, &value, value_t, &mask, mask_t);\n\
        } ' % (field, pro, field, pro, field, field))


def parse_fields_end(pro):
    fo.write('else {\n\
            return xasprintf("unknown %s protocol field: ' % pro)
    fo.write('%s", key);\n\
        }\n\
    }\n')
    fo.write('    omf->protocol.%s = %s;\n\
    omf->mask.%s = masks;\n\
    return NULL;\n\
}\n\n' % (pro, pro, pro))

# ----------------------------------------------------------------------------


def parse_MOD_FIELDS_init():
    fo.write('static char * OVS_WARN_UNUSED_RESULT\n\
parse_MOD_FIELDS(const char *arg, const struct ofpact_prase_params *pp)\n\
{\n\
    char *error = NULL;\n')


def parse_MOD_FIELDS_branch(pro, mod):
    if (mod == 0):
        fo.write('    if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_fields_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))
    else:
        fo.write('else if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_fields_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))


def parse_MOD_FIELDS_end():
    fo.write('else {\n\
        return xasprintf("add unknown protocol");\n\
    }\n\
    return error;\n\
}\n\n')

# ------------------------------------------------------------------------


def parse_fields_out(Li_result):
    for key, val in Li_result.items():
        parse_fields_init(key)
        i = 0
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                parse_fields_branch(key, key_, i)
            i += 1
        parse_fields_end(key)


def MOD_FIELDS_out():
    i = 0
    parse_MOD_FIELDS_init()
    for name in protocol_name:
        parse_MOD_FIELDS_branch(name, i)
        i += 1
    parse_MOD_FIELDS_end()


# --------------------------------parse_check_fields------------------------------------------

def parse_check_fields_init(pro):
    fo.write('char * \n\
parse_check_fields_%s(const char *s, const struct ofpact_parse_params *pp)\n\
{\n\
    char *key, *val;\n\
    struct %s masks;\n\
    struct ofpact_check_crc *occ = ofpact_put_CHECK_CRC(pp->ofpacts);\n\
    int bytes;\n\
    int index;\n\
    uint8_t *mask_t;\n\
\n\
    occ->flag = %s;\n\
    \n\
    memset(&masks, 0, sizeof(masks));\n\
    while(ofputil_parse_key_value(&s, &key, &val)) {\n        '
             % (pro, pro, pro.upper()))


def parse_check_fields_branch(pro, field, mod):
    if (mod == 0):
        fo.write('if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(masks.%s);\n\
            mask_t = (uint8_t*)(&masks.%s);\n\
            memset(mask_t, 0xff, bytes);\n\
        } ' % (field, field, field))
    else:
        fo.write('else if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(masks.%s);\n\
            mask_t = (uint8_t*)(&masks.%s);\n\
            memset(mask_t, 0xff, bytes);\n\
        } ' % (field, field, field))


def parse_check_fields_end(pro):
    fo.write('else if (!strcmp(key, "all")) {\n\
            get_index(%s, &index);\n\
            bytes = crc_ofs[index];\n\
            memset(&masks, 0xff, bytes);\n\
        } else {\n\
            return xasprintf("unknown %s protocol field: ' % (pro.upper(), pro))
    fo.write('%s", key);\n\
        }\n\
    }\n')
    fo.write('    occ->mask.%s = masks;\n\
    return NULL;\n\
}\n\n' % (pro))

# ----------------------------------------------------------------------------


def parse_CHECK_CRC_init():
    fo.write('static char * OVS_WARN_UNUSED_RESULT\n\
parse_CHECK_CRC(const char *arg, const struct ofpact_prase_params *pp)\n\
{\n\
    char *error = NULL;\n')


def parse_CHECK_CRC_branch(pro, mod):
    if (mod == 0):
        fo.write('    if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_check_fields_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))
    else:
        fo.write('else if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_check_fields_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))


def parse_CHECK_CRC_end():
    fo.write('else {\n\
        return xasprintf("check unknown protocol");\n\
    }\n\
    return error;\n\
}\n\n')

# ------------------------------------------------------------------------


def parse_check_fields_out(Li_result):
    for key, val in Li_result.items():
        parse_check_fields_init(key)
        i = 0
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                parse_check_fields_branch(key, key_, i)
            i += 1
        parse_check_fields_end(key)


def CHECK_CRC_out():
    i = 0
    parse_CHECK_CRC_init()
    for name in protocol_name:
        parse_CHECK_CRC_branch(name, i)
        i += 1
    parse_CHECK_CRC_end()
    
    
    
    
# --------------------------------parse_calc_fields------------------------------------------

def parse_calc_fields_init(pro):
    fo.write('char * \n\
parse_calc_fields_%s(const char *s, const struct ofpact_parse_params *pp)\n\
{\n\
    char *key, *val;\n\
    struct %s masks;\n\
    struct ofpact_check_crc *occ = ofpact_put_CALC_CRC(pp->ofpacts);\n\
    int bytes;\n\
    int index;\n\
    uint8_t *mask_t;\n\
\n\
    occ->flag = %s;\n\
    \n\
    memset(&masks, 0, sizeof(masks));\n\
    while(ofputil_parse_key_value(&s, &key, &val)) {\n        '
             % (pro, pro, pro.upper()))


def parse_calc_fields_branch(pro, field, mod):
    if (mod == 0):
        fo.write('if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(masks.%s);\n\
            mask_t = (uint8_t*)(&masks.%s);\n\
            memset(mask_t, 0xff, bytes);\n\
        } ' % (field, field, field))
    else:
        fo.write('else if (!strcmp(key, "%s")) {\n\
            bytes = sizeof(masks.%s);\n\
            mask_t = (uint8_t*)(&masks.%s);\n\
            memset(mask_t, 0xff, bytes);\n\
        } ' % (field, field, field))


def parse_calc_fields_end(pro):
    fo.write('else if (!strcmp(key, "all")) {\n\
            get_index(%s, &index);\n\
            bytes = crc_ofs[index];\n\
            memset(&masks, 0xff, bytes);\n\
        } else {\n\
            return xasprintf("unknown %s protocol field: ' % (pro.upper(), pro))
    fo.write('%s", key);\n\
        }\n\
    }\n')
    fo.write('    occ->mask.%s = masks;\n\
    return NULL;\n\
}\n\n' % (pro))

# ----------------------------------------------------------------------------


def parse_CALC_CRC_init():
    fo.write('static char * OVS_WARN_UNUSED_RESULT\n\
parse_CALC_CRC(const char *arg, const struct ofpact_prase_params *pp)\n\
{\n\
    char *error = NULL;\n')


def parse_CALC_CRC_branch(pro, mod):
    if (mod == 0):
        fo.write('    if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_calc_fields_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))
    else:
        fo.write('else if (!strncmp(arg, "%s", strlen("%s"))) {\n\
        error = parse_calc_fields_%s(arg+strlen("%s"), pp);\n\
    } ' % (pro, pro, pro, pro))


def parse_CALC_CRC_end():
    fo.write('else {\n\
        return xasprintf("calculate unknown protocol");\n\
    }\n\
    return error;\n\
}\n\n')

# ------------------------------------------------------------------------


def parse_calc_fields_out(Li_result):
    for key, val in Li_result.items():
        parse_calc_fields_init(key)
        i = 0
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                parse_calc_fields_branch(key, key_, i)
            i += 1
        parse_calc_fields_end(key)


def CALC_CRC_out():
    i = 0
    parse_CALC_CRC_init()
    for name in protocol_name:
        parse_CALC_CRC_branch(name, i)
        i += 1
    parse_CALC_CRC_end()


#-----------------------------------------------------------------------------------
def end_output():
    fo.write('static char * OVS_WARN_UNUSED_RESULT\n\
ofpact_parse(enum ofpact_type type, char *value,\n\
             const struct ofpact_parse_params *pp)\n\
{\n\
    switch (type) {\n\
#define OFPACT(ENUM, STRUCT, MEMBER, NAME)                              \ \n\
        case OFPACT_##ENUM:                                             \ \n\
            return parse_##ENUM(value, pp);\n\
        OFPACTS\n\
#undef OFPACT\n\
    default:\n\
        OVS_NOT_REACHED();\n\
    }\n\
}\n\
\n\
\n\
static bool\n\
ofpact_type_from_name(const char *name, enum ofpact_type *type)\n\
{\n\
#define OFPACT(ENUM, STRUCT, MEMBER, NAME)                            \ \n\
    if (!strcasecmp(name, NAME)) {                                    \ \n\
        *type = OFPACT_##ENUM;                                          \ \n\
        return true;                                                    \ \n\
    }\n\
    OFPACTS\n\
#undef OFPACT\n\
\n\
    return false;\n\
}')
    
#-------------------第二次生成的代码------------------#    
    
#------------------ds_put_pmap-----------#
str_ds_put_pmap_pmap1 = '''void 
ds_put_pmap(struct ds *ds, union protocols *value_, union protocols *mask_)
{
    struct pmap value = value_->pmap;
    struct pmap mask = mask_->pmap;

'''
    
str_ds_put_pmap_field = '''    if (*(uint8_t*)&mask.FILED) {
        ds_put_format(ds, "FILED=");
        ds_put_hex(ds, &value.FILED, sizeof(value.FILED));
        ds_put_char(ds, ',');
    }\n'''    
    
str_ds_put_pmap_pmap2 = '''}\n\n'''

def create_str_ds_put_pmap_mid(Li_result):
    for key, val in Li_result.items():
        fo.write(str_ds_put_pmap_pmap1.replace('pmap', key)) 
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                fo.write(str_ds_put_pmap_field.replace('FILED', key_)) 
        fo.write(str_ds_put_pmap_pmap2)
                
    
#------------------format_ADD_PROTOCOL-----------#
str_format_ADD_PROTOCOL_start = '''\n\nstatic void
format_ADD_PROTOCOL(const struct ofpact_add_protocol *a,
              const struct ofpact_format_params *fp)
{
    ds_put_format(fp->s, "%sadd_protocol(", colors.param);
    union protocols mask;
    memset(&mask, 0xff, sizeof(mask));
    '''
    
str_format_ADD_PROTOCOL_mid = '''   switch (a->flag) {
    case PMAP:
        ds_put_format(fp->s, "pmap, ");
        ds_put_pmap(fp->s, &a->protocol, &mask);
        break;
    }'''
    
str_format_ADD_PROTOCOL_end = '''    ds_put_format(fp->s, ")%s ", colors.end);
}'''

def create_format_ADD_PROTOCOL_start():
    fo.write(str_format_ADD_PROTOCOL_start)
    
def create_format_ADD_PROTOCOL_mid(Li_result):
    for key, val in Li_result.items():
        str_format_ADD_PROTOCOL_mid1 = str_format_ADD_PROTOCOL_mid.replace('PMAP', key.upper())
        str_format_ADD_PROTOCOL_mid1 = str_format_ADD_PROTOCOL_mid1.replace('pmap', key)
        fo.write(str_format_ADD_PROTOCOL_mid1)
                
def create_format_ADD_PROTOCOL_end():
    fo.write(str_format_ADD_PROTOCOL_end)
    
    
#------------------format_DEL_PROTOCOL-----------#
str_format_DEL_PROTOCOL_start = '''\n\nstatic void
format_DEL_PROTOCOL(const struct ofpact_del_protocol *a,
              const struct ofpact_format_params *fp)
{
    char *protocol;
    switch (a->flag) {
'''
    
str_format_DEL_PROTOCOL_mid = '''    case PMAP:
        protocol = xstrdup("pmap");
        break;
'''
    
str_format_DEL_PROTOCOL_end = '''    default:
        protocol = xstrdup(" ");
    }
    ds_put_format(fp->s, "%sdel_protocol:%s%s ", colors.param, protocol, colors.end);
}\n\n'''

def create_format_DEL_PROTOCOL_start():
    fo.write(str_format_DEL_PROTOCOL_start)
    
def create_format_DEL_PROTOCOL_mid(Li_result):
    for key, val in Li_result.items():
        str_format_DEL_PROTOCOL_mid1 = str_format_DEL_PROTOCOL_mid.replace('PMAP', key.upper())
        str_format_DEL_PROTOCOL_mid1 = str_format_DEL_PROTOCOL_mid1.replace('pmap', key)
        fo.write(str_format_DEL_PROTOCOL_mid1)
                
def create_format_DEL_PROTOCOL_end():
    fo.write(str_format_DEL_PROTOCOL_end)
    
    
#------------------format_MOD_PROTOCOL-----------#
str_format_MOD_PROTOCOL_start = '''static void
format_MOD_FIELDS(const struct ofpact_mod_fields *a,
              const struct ofpact_format_params *fp)
{
    ds_put_format(fp->s, "%smod_fields(", colors.param);

'''
    
str_format_MOD_PROTOCOL_mid = '''   switch (a->flag) {
    case PMAP:
        ds_put_format(fp->s, "pmap, ");
        ds_put_pmap(fp->s, &a->protocol, &a->mask);
        break;
    }'''
    
str_format_MOD_PROTOCOL_end = '''    ds_put_format(fp->s, ")%s ", colors.end);
}'''

def create_format_MOD_PROTOCOL_start():
    fo.write(str_format_MOD_PROTOCOL_start)
    
def create_format_MOD_PROTOCOL_mid(Li_result):
    for key, val in Li_result.items():
        str_format_MOD_PROTOCOL_mid1 = str_format_MOD_PROTOCOL_mid.replace('PMAP', key.upper())
        str_format_MOD_PROTOCOL_mid1 = str_format_MOD_PROTOCOL_mid1.replace('pmap', key)
        fo.write(str_format_MOD_PROTOCOL_mid1)
                
def create_format_MOD_PROTOCOL_end():
    fo.write(str_format_MOD_PROTOCOL_end)
    
    
#------------------parse_MOVE_PROTOCOL-----------#
str_parse_MOVE_PROTOCOL_start = '''static char * OVS_WARN_UNUSED_RESULT
parse_MOVE_PROTOCOL(const char *arg, const struct ofpact_parse_params *pp)
{   
    char *key, *val;
    struct ofpact_move_protocol *omp = ofpact_put_MOVE_PROTOCOL(pp->ofpacts);
    // VLOG_INFO("parse_MOVE_PROTOCOL: arg: %s", arg);
    while(ofputil_parse_key_value(&arg, &key, &val)) {
        enum pmap *p;
        if (!strncmp(key, "dst", 3)) {
            p = &omp->dst;
        } else if (!strncmp(key, "src", 3)) {
            p = &omp->src;
        } else {
            return xasprintf("please type dst or src");
        }

        '''
    
str_parse_MOVE_PROTOCOL_mid_if = '''if (!strncmp(val, "pmap", strlen("pmap"))) {
            *p = PMAP;
        } '''
        
str_parse_MOVE_PROTOCOL_mid_elif = '''else if (!strncmp(val, "pmap", strlen("pmap"))) {
            *p = PMAP;
        } '''
    
str_parse_MOVE_PROTOCOL_end = ''' else {
            return xasprintf("move_protocol: unknown protocol name %s", val);
        }
    }

    if (!omp->dst || !omp->src) {
        return xasprintf("move_protocol: please give dst and src");
    }
    
    if (omp->dst == omp->src) {
        return xasprintf("move_protocol: do not give the same protocol");
    }

    return NULL;
}'''

if_flag = 0

def create_parse_MOVE_PROTOCOL_start():
    fo.write(str_parse_MOVE_PROTOCOL_start)
    
def create_parse_MOVE_PROTOCOL_mid(Li_result):
    global if_flag
    for key, val in Li_result.items():
        if(if_flag == 0):
            str_parse_MOVE_PROTOCOL_mid1_if1 = str_parse_MOVE_PROTOCOL_mid_if.replace('pmap', key)
            str_parse_MOVE_PROTOCOL_mid1_if1 = str_parse_MOVE_PROTOCOL_mid1_if1.replace('PMAP', key.upper())
            fo.write(str_parse_MOVE_PROTOCOL_mid1_if1)
            if_flag = 1
        else:
            str_parse_MOVE_PROTOCOL_mid_elif1 = str_parse_MOVE_PROTOCOL_mid_elif.replace('pmap', key)
            str_parse_MOVE_PROTOCOL_mid_elif1 = str_parse_MOVE_PROTOCOL_mid_elif1.replace('PMAP', key.upper())
            fo.write(str_parse_MOVE_PROTOCOL_mid_elif1)
                
def create_parse_MOVE_PROTOCOL_end():
    fo.write(str_parse_MOVE_PROTOCOL_end)