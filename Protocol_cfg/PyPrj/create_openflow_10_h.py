import yaml
import json

readpath = './cfile/flow.h'
writepath = './cfile/openflow-1.0.h'

sstart = '''struct ofp10_match_mof {

     ovs_be16 in_port;
     uint8_t pad[6];
     uint8_t data[248];
};
'''

def create_struct_ofp10_match_mof(num):
    with open(readpath, 'r') as src, open(writepath, 'w') as dst:
        sstart_r = sstart.replace('248', str(num))
        dst.write(sstart_r)
        # src.seek(47) 
        
        # content = src.read(2000) 
        
        # dst.write(content)



