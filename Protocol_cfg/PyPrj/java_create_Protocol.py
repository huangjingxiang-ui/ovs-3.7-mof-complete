import yaml

outputpath = './javafile/instructions/protocol/Protocol.java'
outputpath1 = './javafile/instructions/protocol/Specific_Protocol.java'

sp_start = '''package org.onosproject.net.flow.instructions.protocol;

import java.util.Objects;
import java.util.Set;

import io.netty.buffer.Unpooled;
import io.netty.buffer.ByteBuf;
import org.onosproject.net.packet.mof.*;

public class Specific_Protocol {
    private static Specific_Protocol instance = new Specific_Protocol();\n'''

sp_data0 = '    private Set<Integer> white_list = Set.of('

sp_mid = '''    public static synchronized Specific_Protocol getInstance(){
        return instance;
    }

    public boolean contains(int type){
        return white_list.contains(type);
    }\n\n'''


start = '''package org.onosproject.net.flow.instructions.protocol;

import io.netty.buffer.ByteBuf;

public interface Protocol {
    final int UNKNOW = 0;\n'''

mid1 = '''\n    public static String ProtocolFormatByType(int type) {
        switch(type){\n'''

mid2 = '''            default:
                return "UNKONW_PROTOCOL";
        }
    }

    public static int ProtocolFormatByString(String type) {
        switch(type){\n'''

end = '''            default:
                return UNKNOW;
        }
    }

    void write(ByteBuf bb);
    void writeMask(ByteBuf bb);
}'''

data1 = '    final int MAC = 1 << 0;\n'

data2 = '''            case MAC:
                return "MAC_PROTOCOL";\n'''

data3 = '''            case "MAC_PROTOCOL":
                return MAC;\n'''

index = 0

def create_Protocol_start():
    with open(outputpath, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(start)
    with open(outputpath1, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(sp_start)
    with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
        result = yaml.load(f.read(), Loader=yaml.FullLoader)

    sp_data0r = sp_data0
    if result['Specific'] is not None:
        for pro_type in result['Specific'].values():
            sp_data0r = sp_data0r + pro_type + ','
    if sp_data0r[-1] == ',':
        sp_data0r = sp_data0r[0:-1]
    sp_data0r = sp_data0r + ');\n'
    with open(outputpath1, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(sp_data0r)
    with open(outputpath1, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(sp_mid)
                


def create_Protocol_end():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(end)
    with open(outputpath1, mode='a', encoding='utf-8') as file_obj:
        file_obj.write("}")


def create_Protocol_mid1():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid1)


def create_Protocol_mid2():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid2)


def create_Protocol1(Li_result):
    global index 
    for key, val in Li_result.items():
        if key != 'srv6':
            data1r = data1.replace('MAC', key.upper())
            data1r = data1r.replace('0', str(index))
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data1r)
            index += 1
        else:
            for key_, val_ in val.items():
                if key_ == "srv6_segmentlist": 
                    srv6_segl = int(val_['size']) % 128
                    for i in range(0, srv6_segl):
                        data1r = data1.replace('MAC', key.upper() + '_' + str(i + 1))
                        data1r = data1r.replace('0', str(index))
                        with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data1r)
                        index += 1


def create_Protocol2(Li_result):
    for key, val in Li_result.items():
        if key != 'srv6':
            data2r = data2.replace('MAC', key.upper())
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data2r)
        else:
            for key_, val_ in val.items():
                if key_ == "srv6_segmentlist": 
                    srv6_segl = int(val_['size']) % 128
                    for i in range(0, srv6_segl):
                        data2r = data2.replace('MAC', key.upper() + '_' + str(i + 1))
                        with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data2r)


def create_Protocol3(Li_result):
    for key, val in Li_result.items():
        if key != 'srv6':
            data3r = data3.replace('MAC', key.upper())
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data3r)
        else:
            for key_, val_ in val.items():
                if key_ == "srv6_segmentlist": 
                    srv6_segl = int(val_['size']) % 128
                    for i in range(0, srv6_segl):
                        data3r = data3.replace('MAC', key.upper() + '_' + str(i + 1))
                        with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data3r) 
                            