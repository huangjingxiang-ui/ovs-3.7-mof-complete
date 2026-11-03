import yaml
import json

align = '   '
flow_mof_start = 'struct flow_mof {\n'

def create_flow_mof():
    fo = open('./cfile/flow.h', 'a', encoding='utf-8')
    