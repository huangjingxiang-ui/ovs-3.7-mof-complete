# 通过main.yml中的每层的头的名称找到对应的文件

import os
import sys
import yaml
import createflow
import createparser
import create_minifow_extract
import createhash
import createmetaflow_c
import create_packets_c
import create_ofp_actions_c
import create_ofproto_dpif_xlate_c
import create_odp_execute_c
import create_flowmap_set_c
import create_openflow_10_h
import create_match_c
import java_create_Criteria_java
import java_create_Criteria_java_class
import java_create_CriterionCodec
import java_create_CriterionParser
import java_create_Protocol
import java_create_Protocol_class
import java_create_Instructions
import create_add_flow
import java_create_MOFFlow
import java_create_MOFlayer

# new_dir = "/home/ubuntu/下载/mof/Protocol_cfg/PyPrj"
# os.chdir(new_dir)

with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
    result = yaml.load(f.read(), Loader=yaml.FullLoader)
    layer_list = list(result.keys())

aLayer = [element for element in layer_list if "Layer" in element]
# 判断是否为存在的协议
aExistHeader = [' ']
Ttype = {}
Ttype_size = {}

createflow.vFlowInit()
createparser.vParserInit()
createhash.vHashInit()
createmetaflow_c.vSetInit()
createflow.vCreateSmallStructInit()
createflow.vCreateUnionProtolsInit()
createflow.vCreateEnumPmapInit()
createflow.vOriginFlowInit()
padsize = 0
num = 1
srv6num = 0
srv6num_use = 0

for la in aLayer:
    Layeri = result[la] 
    for name in Layeri:
        if (None != name):
            # 存在的协议直接copy
            if (name in aExistHeader):
                padsize = createflow.vCreateFlow(None, name)
                createparser.vCreateParser(None, name, padsize)
            # 新的协议再去生成
            else:
                newname = '../header_describe/' + name + '.yml'
                # print(newname)
                with open(newname, 'r', encoding='utf-8') as f:
                    Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                    #print(Li_result)
                    padsize, srv6num = createflow.vCreateFlow(Li_result, None)
                    if (srv6num != 0):
                        srv6num_use = srv6num
                    createmetaflow_c.vCreateSet(Li_result, srv6num)
                    createparser.vCreateParser(Li_result, None, padsize)

            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                # 生成hash
                createhash.vCreateHash(Li_result, None)

srv6num = srv6num_use
flow_bits = createflow.vFlowEnd()
createhash.vHashFinish()
createmetaflow_c.vSetFinish()
createflow.vCreateUnionProtolsEnd(srv6num)
createflow.vCreateEnumPmapEnd(srv6num)
flow_mof_bits = createflow.vOriginFlowEnd()
total_bits = flow_bits if flow_bits > flow_mof_bits else flow_mof_bits

# ------------ miniflow_extract_mof & packets.c & ofp-actions.c & ofproto_dpif_xlate.c---------------
# ------------srv6 ofproto_dpif_xlate.c & odp-excute.c & packets.c ----------------------------------
srv6_exsist = 0


def protocol_and_fields_handler(func0, func1, func2, srv6_all):
    if (func0 != None):
        func0()
    if (func1 != None):
        for la in aLayer:
            Layeri = result[la]
            for name in Layeri:
                # print(name)
                if (None != name):
                    if (name == 'srv6'):
                        if (srv6_all == 1):
                            # print("is srv6")
                            for i in range(srv6num):
                                newname = '../header_describe/srv6_n/' + \
                                    name + '_' + str(i+1) + '.yml'
                                with open(newname, 'r', encoding='utf-8') as f:
                                    Li_result = yaml.load(
                                        f.read(), Loader=yaml.FullLoader)
                                    func1(Li_result)
                        else:
                            newname = '../header_describe/srv6_n/' + \
                                name + '_' + str(srv6num) + '.yml'
                            with open(newname, 'r', encoding='utf-8') as f:
                                Li_result = yaml.load(
                                    f.read(), Loader=yaml.FullLoader)
                                func1(Li_result)
                    else:
                        newname = '../header_describe/' + name + '.yml'
                        with open(newname, 'r', encoding='utf-8') as f:
                            Li_result = yaml.load(
                                f.read(), Loader=yaml.FullLoader)
                            func1(Li_result)
    if (func2 != None):
        func2()


# first = next(iter(result[aLayer[0]]))
# print(first)
for la in aLayer:
    Layeri = result[la]
    for name in Layeri:
        if (None != name):
            if (name == 'srv6'):
                srv6_exsist = 1
            create_packets_c.addname(name)
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                first_field = next(iter((next(iter(Li_result.items()))[1])))
                create_packets_c.crc_handler(Li_result)
                create_ofproto_dpif_xlate_c.add_first_field(first_field)
                create_minifow_extract.Ttype_get(
                    Li_result, name, Ttype, Ttype_size)

for key, val in Ttype.items():
    if (val != None):
        first_type = key
        break
create_minifow_extract.create_extract(Ttype, Ttype_size, first_type, srv6num)
create_packets_c.create_packets_c_1(srv6num)
create_packets_c.create_packets_c_2(srv6num)
create_ofproto_dpif_xlate_c.create_flow_mof_protocol_ofs(srv6num)
create_ofproto_dpif_xlate_c.create_xlate_srv6(srv6_exsist)
create_odp_execute_c.create_case_seg_routing(srv6_exsist)
create_openflow_10_h.create_struct_ofp10_match_mof(total_bits // 8 - 536)

protocol_and_fields_handler(None, create_ofp_actions_c.parse_protocol_out,
                            create_ofp_actions_c.ADD_DEL_PROTOCOL_out, 1)
protocol_and_fields_handler(None, create_ofp_actions_c.parse_fields_out, create_ofp_actions_c.MOD_FIELDS_out, 1)
protocol_and_fields_handler(None, create_ofp_actions_c.parse_check_fields_out, create_ofp_actions_c.CHECK_CRC_out, 1)
protocol_and_fields_handler(None, create_ofp_actions_c.parse_calc_fields_out, create_ofp_actions_c.CALC_CRC_out, 1)
protocol_and_fields_handler(None, create_ofp_actions_c.create_str_ds_put_pmap_mid, None, 1)
protocol_and_fields_handler(create_ofp_actions_c.create_parse_MOVE_PROTOCOL_start,
                            create_ofp_actions_c.create_parse_MOVE_PROTOCOL_mid, 
                            create_ofp_actions_c.create_parse_MOVE_PROTOCOL_end, 1)

protocol_and_fields_handler(create_ofp_actions_c.create_format_ADD_PROTOCOL_start,
                            create_ofp_actions_c.create_format_ADD_PROTOCOL_mid, create_ofp_actions_c.create_format_ADD_PROTOCOL_end, 1)
protocol_and_fields_handler(create_ofp_actions_c.create_format_DEL_PROTOCOL_start,
                            create_ofp_actions_c.create_format_DEL_PROTOCOL_mid, create_ofp_actions_c.create_format_DEL_PROTOCOL_end, 1)
protocol_and_fields_handler(create_ofp_actions_c.create_format_MOD_PROTOCOL_start,
                            create_ofp_actions_c.create_format_MOD_PROTOCOL_mid, create_ofp_actions_c.create_format_MOD_PROTOCOL_end, 1)
protocol_and_fields_handler(create_match_c.create_match_format_mof_start,
                            create_match_c.create_match_format_mof_mid, create_match_c.create_match_format_mof_end, 0)
protocol_and_fields_handler(None, create_flowmap_set_c.create_flowmap_set, None, 1)

# ---------------------------------------javafile----------------------------------------------------
max_len = 0

for la in aLayer:
    Layeri = result[la]
    for name in Layeri:
        if (None != name):
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                if (next(iter(Li_result)) == 'srv6'):
                    with open('../header_describe/srv6_n/srv6_' + str(srv6num) + '.yml', 'r', encoding='utf-8') as f_:
                        Li_result = yaml.load(
                            f_.read(), Loader=yaml.FullLoader)
                    Li_key = next(iter(Li_result))
                    Li_result_ = {}
                    Li_result_[Li_key[:4]] = Li_result[next(iter(Li_result))]
                    Li_result = Li_result_.copy()
                java_create_Criteria_java.create_public_interface_TrafficSelector(
                    Li_result)
                java_create_Criteria_java.create_public_final_class_Criteria(
                    Li_result)
                java_create_Criteria_java.create_Criterion_java(Li_result)
                java_create_Criteria_java.create_DefaultTrafficSelector1(
                    Li_result)
                java_create_Criteria_java.create_DefaultTrafficSelector2_1(
                    Li_result, total_bits)
                java_create_Criteria_java.create_DefaultTrafficSelector2_2(
                    Li_result)
                for key, val in Li_result.items():
                    length = 0
                    for key_, val_ in val.items():
                        if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                            if key_ == "srv6_segmentlist":
                                length += val_['size'] % 128 * 128
                            else:
                                length += val_['size']
                    if max_len < length:
                        max_len = length

layer_label = 0
java_create_MOFFlow.create_MOFFlow_start()
java_create_Protocol.create_Protocol_start()
java_create_Instructions.create_Instructions_start()

for la in aLayer:
    Layeri = result[la]
    # print(Layeri)
    if la == 'Layer3':
        layer_label = 1
    if layer_label and Layeri[0] != None:
        java_create_MOFlayer.create_MOFlayer(Layeri, la[-1], result[layer_list[layer_list.index(la) - 1]])
        java_create_MOFFlow.create_MOFFlow(Layeri, la[-1], 0)
    for name in Layeri:
        if (None != name):
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                java_create_Criteria_java_class.create_public_final_class_implements_Criterion(
                    Li_result)
                java_create_Protocol.create_Protocol1(Li_result)
                java_create_Protocol_class.create_Protocol_class(Li_result, max_len // 8)
                java_create_Instructions.create_Instructions1(Li_result, 1)

layer_label = 0
java_create_MOFFlow.create_MOFFlow_mid1()
java_create_Protocol.create_Protocol_mid1()
java_create_Instructions.create_Instructions_mid1()
java_create_CriterionCodec.create_CriterionCodec_start()

for la in aLayer:
    Layeri = result[la]
    if la == 'Layer3':
        layer_label = 1
    if layer_label and Layeri[0] != None:
        java_create_MOFFlow.create_MOFFlow(Layeri, la[-1], 1)
    for name in Layeri:
        if (None != name):
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                java_create_CriterionCodec.create_CriterionCodec(Li_result)
                java_create_Protocol.create_Protocol2(Li_result)
                java_create_Instructions.create_Instructions2(Li_result, 1)

layer_label = 0
java_create_Protocol.create_Protocol_mid2()
java_create_Instructions.create_Instructions_mid2()
java_create_CriterionCodec.create_CriterionCodec_end()

for la in aLayer:
    Layeri = result[la]
    if la == 'Layer3':
        layer_label = 1
    if layer_label and Layeri[0] != None:
        java_create_MOFFlow.create_MOFFlow(Layeri, la[-1], 2)
    for name in Layeri:
        if (None != name):
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                java_create_CriterionCodec.create_CriterionCodecHelper(
                    Li_result)         
                java_create_Protocol.create_Protocol3(Li_result)             
                java_create_Instructions.create_Instructions1(Li_result, 3)

layer_label = 0
java_create_MOFFlow.create_MOFFlow_mid2()
java_create_Protocol.create_Protocol_end()
java_create_Instructions.create_Instructions_mid3()
java_create_CriterionCodec.create_CriterionCodecHelper_end()

for la in aLayer:
    Layeri = result[la]
    if la == 'Layer4':
        layer_label = 1
    if layer_label and Layeri[0] != None:
        java_create_MOFFlow.create_MOFFlow(Layeri, la[-1], 3)
    for name in Layeri:
        if (None != name):
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                java_create_CriterionCodec.create_CriterionCodecHelper1(
                    Li_result)
                java_create_Instructions.create_Instructions2(Li_result, 3)

layer_label = 0
java_create_MOFFlow.create_MOFFlow_mid3()
java_create_CriterionCodec.create_CriterionCodecHelper1_end()
java_create_Instructions.create_Instructions_end()

java_create_CriterionParser.create_CriterionParser_start()

for la in aLayer:
    Layeri = result[la]
    if la == 'Layer3':
        layer_label = 1
    if layer_label and Layeri[0] != None:
        java_create_MOFFlow.create_MOFFlow(Layeri, la[-1], 4)
    for name in Layeri:
        if (None != name):
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                java_create_CriterionParser.create_CriterionParser(Li_result)

java_create_MOFFlow.create_MOFFlow_end()
java_create_CriterionParser.create_CriterionParser_end()

for la in aLayer:
    Layeri = result[la]
    for name in Layeri:
        if (None != name):
            with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
                java_create_CriterionParser.create_CriterionParser1(Li_result)

java_create_CriterionParser.create_CriterionParser_end1()



# ---------------------------------------FlowTable_Config----------------------------------------------------
FlowCfg_Path = "../FlowTable_Config"
Switch_Paths = {}  

current_directory = os.getcwd()

for filename in os.listdir(FlowCfg_Path):
    if filename.endswith('.yml'):  # 
        file_path = os.path.join(FlowCfg_Path, filename)  # 
        relative_path = os.path.relpath(file_path, current_directory)  # 
        Switch_Paths[os.path.splitext(filename)[0]] =relative_path

for switch, path in Switch_Paths.items():
    with open(path, 'r', encoding='utf-8') as f:
        Flow = yaml.safe_load(f.read())
        create_add_flow.add_flow_main(switch,Flow) 
