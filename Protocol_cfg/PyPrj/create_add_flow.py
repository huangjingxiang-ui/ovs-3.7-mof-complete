import yaml

# arp:dl_type=0x0806
# bgp:dl_type=0x0804
# ndp：ipv6_next_header=0x3a

scripts_add_port_flow = '''../scripts_add_port_flow'''
output_path = ''''''

num_vlan = 0
special_pkg = [['dl_type', '0x0806'], ['dl_type', '0x0804'], ['ipv6_e_type', '0x3a'], ['ipv6_dst_e', None], ['ip_daddr_e', None]]

def addr_trans(addr):
    if(addr == 'ipv6_dst_e'):
        return 'Ipv6_addr'
    if(addr == 'ip_daddr_e'):
        return 'Ipv4_addr'
    return None

def get_key(dict):
    return next(iter(dict))

def get_val(dict):
    return dict[next(iter(dict))]

str_add_port = "ovs-vsctl add-port switch port__\n"
str_add_tap = "ip tuntap del dev tapname__ mode tap\nip tuntap add dev tapname__ mode tap\nifconfig tapname__ up\n"

Flow_Table0_Type1_withoutvlan = '''ovs-ofctl add-flow-mof switch "in_port=p_in_,__field=field__,priority=3,table=0,actions=output:p_out_"\n'''
Flow_Table0_Type1_withvlan = '''ovs-ofctl add-flow-mof switch "in_port=p_in_,vlan1_qid=vlan__,__field=field__,priority=3,table=0,actions=output:p_out_"\n'''
Flow_Table0_Type2_withoutvlan = '''ovs-ofctl add-flow-mof switch "in_port=p_in_,priority=3,table=0,actions=output:p_out_"\n'''
Flow_Table0_Type2_withvlan = '''ovs-ofctl add-flow-mof switch "in_port=p_in_,priority=3,table=0,actions=output:p_out_"\n'''
Flow_Table0_Type3 = '''ovs-ofctl add-flow-mof switch "in_port=p_in_,__field=field__,priority=2,table=0,actions=goto_table:table__"\n'''
Flow_Table0_Type4 = '''ovs-ofctl add-flow-mof switch "in_port=p_in_,priority=1,table=0,actions=goto_table:34"\n'''
Flow_Table0_Type5 = '''ovs-ofctl add-flow-mof switch "in_port=p_in_,priority=1,table=0,actions=goto_table:1"\n'''

Flow_Table1_end = '''ovs-ofctl add-flow-mof switch "ipv6_dst_e=SID__,vlan1_qid=vlan__,priority=1,table=1,actions=seg_routing,goto_table:table__"\n'''
Flow_Table1_endx = '''ovs-ofctl add-flow-mof switch "ipv6_dst_e=SID__,priority=1,table=1,actions=seg_routing,groupid__"\n'''
Flow_Table1_endop = '''ovs-ofctl add-flow-mof switch "ipv6_dst_e=SID__,in_port=p_in_,priority=1,table=1,actions=output:p_out_"\n'''
Flow_Table1_endop_withvlan = '''ovs-ofctl add-flow-mof switch "ipv6_dst_e=SID__,in_port=p_in_,vlan1_qid=vlan__,priority=1,table=1,actions=output:p_out_"\n'''
Flow_Table1_enddt = '''ovs-ofctl add-flow-mof switch "ipv6_dst_e=SID__,vlan1_qid=vlan__,priority=1,table=1,actions=seg_routing,goto_table:table__"\n'''
Flow_Table1_enddx6 = '''ovs-ofctl add-flow-mof switch "ipv6_dst_e=SID__,priority=1,table=1,actions=seg_routing,groupid__"\n'''
Flow_Table1_enddx4 = '''ovs-ofctl add-flow-mof switch "ipv6_dst_e=SID__,priority=1,table=1,actions=mod_fields(dl, dl_type=0x0800),seg_routing,groupid__"\n'''

Flow_Table1_NotLocal_all = '''ovs-ofctl add-flow-mof switch "priority=0,table=1,actions=groupid__"\n'''
Flow_Table1_Notlocal_set = '''ovs-ofctl add-flow-mof switch "__field=field__,priority=0,table=1,actions=groupid__"\n'''

Flow_Table2_34 = '''ovs-ofctl add-flow-mof switch "__field=field__,table=tableid__,actions=actions__"\n'''
Flow_ADD_group = '''ovs-ofctl add-group switch "group_id=groupid__, type=all, bucket=bucket__"\n'''
outputstr = "mod_dl_dst=dst__, mod_dl_src=src__, output:p_out_"

gflow = {}

def output_handle(port, fstr:str):
    global gflow
    # print(str)
    if(port in gflow['Static_ARP'].keys()):
        out1 = outputstr.replace("dst__", gflow['Static_ARP'][port])
        out2 = out1.replace("src__", gflow['Mac_addr'])
        out3 = out2.replace("p_out_", port)
        ret = fstr.replace("output:p_out_", out3)
    else:
        ret = fstr.replace("p_out_", port)
    return ret


def Table2_34_gen(name, flow, f):
    mod1 = Flow_Table2_34.replace('''switch''', name)
    for key, val in flow.items():
        if('Table' in key and 'SID' not in key):
            annot = '#' + key + '\n'
            f.write(annot)
            if(val != None):
                for protc, addrs in val.items():
                    if(addrs != None):
                        for addr, actions in addrs.items():
                            if(type(actions) == list):
                                mod2 = mod1.replace('''__field''', protc)
                                mod3 = mod2.replace('''field__''', addr)
                                mod4 = mod3.replace('''tableid__''', key[5:])
                                act_wr = ''''''
                                for act in actions:
                                    if('ACTION' in act):
                                        act_wr += flow[act]
                                    else:
                                        act_wr += act
                                    if(actions.index(act) != len(actions)-1):
                                        act_wr += ','
                                f.write(mod4.replace('''actions__''', act_wr))
                            else:
                                mod2 = mod1.replace('''__field''', protc)
                                mod3 = mod2.replace('''field__''', addr)
                                mod4 = mod3.replace('''tableid__''', key[5:])
                                if('ACTION' in actions):
                                        mod5 = mod4.replace('''actions__''', flow[actions])
                                else:
                                        mod5 = mod4.replace('''actions__''', actions)
                                f.write(mod5)
                f.write('\n')

def get_vlan_num(flow):
    global num_vlan
    for key, val in flow.items():
        if (key == 'Vlan'):
            num_vlan = len(val)

def SID_func_gen(func, SID, name, vlans, ports, f):
    if(SID == None):
        return
    if(func == 'END'):
        f.write('''#END\n''')
        for sid in SID:
            mod1 = Flow_Table1_end.replace('''switch''', name)
            mod2 = mod1.replace('''SID__''', sid)
            for i in range(len(vlans)):
                mod3 = mod2.replace('''vlan__''', str(vlans[i]))
                mod4 = mod3.replace('''table__''', str(i+2))
                f.write(mod4)
        f.write('''\n''')
    elif(func == 'END.X'):
        f.write('''#END.X\n''')
        for sid in SID:
            mod1 = Flow_Table1_endx.replace('''switch''', name)
            mod2 = mod1.replace('''SID__''', get_key(sid))
            mod3 = mod2.replace('''groupid__''', get_val(sid))
            f.write(mod3) 
        f.write('''\n''')
    elif(func == 'END.OP'):
        f.write('''#END.OP\n''')
        for sid in SID:
            for key, val in ports.items():
                if(type(val) == list):
                    mod1 = Flow_Table1_endop_withvlan.replace('''switch''', name)
                    mod2 = mod1.replace('''SID__''', sid)
                    for i in range(len(val)):
                        mod3 = mod2.replace('''p_in_''', key)
                        mod4 = output_handle(val[i], mod3)
                        mod5 = mod4.replace('''vlan__''', str(vlans[i]))
                        f.write(mod5)
                else:
                    mod1 = Flow_Table1_endop.replace('''switch''', name)
                    mod2 = mod1.replace('''SID__''', sid)
                    mod3 = mod2.replace('''p_in_''', key)
                    mod4 = output_handle(val, mod3)
                    f.write(mod4) 
        f.write('''\n''')
    elif(func == 'END.DT6' or func == 'END.DT4'):
        anot = '#' + func + '\n'
        f.write(anot)
        for sid in SID:
            mod1 = Flow_Table1_enddt.replace('''switch''', name)
            mod2 = mod1.replace('''SID__''', sid)
            for i in range(len(vlans)):
                mod3 = mod2.replace('''vlan__''', str(vlans[i]))
                mod4 = mod3.replace('''table__''', str(i+2))
                f.write(mod4)
        f.write('''\n''')
    elif(func == 'END.DX6'):
        anot = '#' + func + '\n'
        f.write(anot)
        for sid in SID:
            mod1 = Flow_Table1_enddx6.replace('''switch''', name)
            mod2 = mod1.replace('''SID__''', get_key(sid))
            mod3 = mod2.replace('''groupid__''', get_val(sid))
            f.write(mod3) 
        f.write('''\n''')
    elif(func == 'END.DX4'):
        anot = '#' + func + '\n'
        f.write(anot)
        for sid in SID:
            mod1 = Flow_Table1_enddx4.replace('''switch''', name)
            mod2 = mod1.replace('''SID__''', get_key(sid))
            mod3 = mod2.replace('''groupid__''', get_val(sid))
            f.write(mod3) 
        f.write('''\n''')

def Table1_gen(name, flow, f):
    f.write('''#Table1\n''')
    for key, val in flow.items():
        if (key == 'SID_Table'):
            for key_, val_ in val.items():
                SID_func_gen(key_, val_, name, flow['Vlan'], flow['Port'], f)
    f.write('''#Not_Local\n''')
    for key, val in flow.items():
        if(key == 'Not_Local'):
            if('''group''' in val):
                mod1 = Flow_Table1_NotLocal_all.replace('''switch''', name)
                mod2 = mod1.replace('''groupid__''', val)
                f.write(mod2)
            else:
                for protc, addrs in val.items():
                    if(addrs !=None):
                        for addr, grp in addrs.items():
                            mod1 = Flow_Table1_Notlocal_set.replace('''switch''', name)
                            mod2 = mod1.replace('''__field''', protc)
                            mod3 = mod2.replace('''field__''', addr)
                            mod4 = mod3.replace('''groupid__''', grp)
                            f.write(mod4)
    f.write('\n')
                    
def Table0_gen(name, flow, f):
    f.write('''#Table0\n''')
    for key, val in flow.items():
        if (key == 'Port'):
            for key_, val_ in val.items():
                if(type(val_) == list):
                    for i in range(len(val_)):
                        mod0 = Flow_Table0_Type1_withvlan.replace('''vlan__''', str(flow['Vlan'][i]))
                        mod1 = mod0.replace('''switch''', name)
                        mod2 = mod1.replace('''p_in_''', key_)
                        mod3 = output_handle(val_[i], mod2)
                        for spc_type in special_pkg:
                            mod4 = mod3.replace('''__field''', spc_type[0])
                            if (spc_type[1] != None):
                                mod5 = mod4.replace('''field__''', spc_type[1])
                            else:
                                mod5 = mod4.replace('''field__''', flow[addr_trans(spc_type[0])])
                            f.write(mod5)
                else:
                    mod1 = Flow_Table0_Type1_withoutvlan.replace('''switch''', name)
                    mod2 = mod1.replace('''p_in_''', key_)
                    mod3 = output_handle(val_, mod2)
                    for spc_type in special_pkg:
                        mod4 = mod3.replace('''__field''', spc_type[0])
                        if (spc_type[1] != None):
                            mod5 = mod4.replace('''field__''', spc_type[1])
                        else:
                            mod5 = mod4.replace('''field__''', flow[addr_trans(spc_type[0])])
                        f.write(mod5)
    f.write('''\n''')
    for key, val in flow.items():
        if (key == 'Port'):
            for key_, val_ in val.items():
                if(type(val_) == list):
                    for i in range(len(val_)):
                        mod1 = Flow_Table0_Type2_withvlan.replace('''switch''', name)
                        mod2 = mod1.replace('''p_in_''', val_[i])
                        mod3 = output_handle(key_, mod2)
                        f.write(mod3)
                else:
                    mod1 = Flow_Table0_Type2_withoutvlan.replace('''switch''', name)
                    mod2 = mod1.replace('''p_in_''', val_)
                    mod3 = output_handle(key_, mod2)
                    f.write(mod3)
    f.write('''\n''')
    for key, val in flow.items():
        if (key == 'Port'):
            for key_, val_ in val.items():
                if('''PE''' in key_):
                    mod1 = Flow_Table0_Type3.replace('''switch''', name)
                    mod2 = mod1.replace('''p_in_''', key_)
                    for i in range(len(flow['Vlan'])):
                        mod3 = mod2.replace('''__field''', '''vlan1_qid''')
                        mod4 = mod3.replace('''field__''', str(flow['Vlan'][i]))
                        mod5 = mod4.replace('''table__''', str(i+2))
                        f.write(mod5)
    f.write('''\n''')
    for key, val in flow.items():
        if (key == 'Port'):
            for key_, val_ in val.items():
                if('''PE''' in key_):
                    mod1 = Flow_Table0_Type4.replace('''switch''', name)
                    mod2 = mod1.replace('''p_in_''', key_)
                    f.write(mod2)
    f.write('''\n''')
    for key, val in flow.items():
        if (key == 'Port'):
            for key_, val_ in val.items():
                if('''P''' in key_ and '''PE''' not in key_):
                    mod1 = Flow_Table0_Type5.replace('''switch''', name)
                    mod2 = mod1.replace('''p_in_''', key_)
                    f.write(mod2)
    f.write('''\n''')

# def group_output_handle(str1:str):
#     port = 

def group_gen(name, flow, f):
    if(flow['GROUP'] != None):
        f.write('\n#Group Table\n')
        mod1 = Flow_ADD_group.replace("switch", name)
        for grpid, act in flow['GROUP'].items():
            mod2 = mod1.replace("groupid__", grpid[5:])
            # print(mod2)
            if('output' in act):
                mod3 = mod2.replace("bucket__", "output:p_out_")
                mod4 = output_handle(act[7:], mod3)
                f.write(mod4)
            else:
                mod3 = mod2.replace("bucket__", act)
                f.write(mod3)
    f.write("\n")    

def add_port_gen(switchname, Ports, f):
    f.write("#add ports\n")
    for key, val in Ports.items():
        mod1 = str_add_port.replace("switch", switchname)
        mod2 = mod1.replace("port__", key)
        f.write(mod2)
        if(type(val) is list):
            for p in val:
                mod2 = mod1.replace("port__", p)
                f.write(mod2)
        else:
            mod2 = mod1.replace("port__", val)
            f.write(mod2)
    f.write("\n")

def add_tap(ports, f):
    for key, val in ports.items():
            if(type(val) is list):
                for p in val:
                    mod2 = str_add_tap.replace("tapname__", p)
                    f.write(mod2)
            else:
                mod2 = str_add_tap.replace("tapname__", val)
                f.write(mod2)

def add_flow_main(name, flow):
    # print(flow)
    add_port_flow_path = scripts_add_port_flow + '/' + name + '_addflow.sh'
    tap_create_path = scripts_add_port_flow + '/' + name + '_addtap.sh'
    global gflow
    gflow = flow
    with open(add_port_flow_path, mode='w', encoding='utf-8') as f:
        get_vlan_num(flow)
        add_port_gen(name, flow['Port'], f)
        group_gen(name, flow, f)
        Table0_gen(name, flow, f)
        Table1_gen(name, flow, f)
        Table2_34_gen(name, flow, f)
    with open(tap_create_path, mode='w', encoding='utf-8') as f:
        add_tap(flow['Port'], f)