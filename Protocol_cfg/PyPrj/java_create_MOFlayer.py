import yaml

outputpath1 = './javafile/instructions/protocol/Specific_Protocol.java'

sp_data = '''    public void parse_specific_protocolL3(int type, ByteBuf bb, MOFL3Layer l3){
        throw new UnsupportedOperationException("L3Layer has no specific protocol yet");
    }\n\n'''

sp_data1 = '        switch (type) {\n'
sp_data2 = '''            case 0x12:
                throw new UnsupportedOperationException("L3Layer type 0x12 is undefind");\n'''

sp_data3 = '''            default:
                break;
        }
    }\n\n'''

srv6_data1 = '''            case 0x12:
                int start = bb.readerIndex();
                bb.skipBytes(1);
                int hdr_ext_len = bb.readByte();
                bb.readerIndex(start);

                switch (hdr_ext_len) {\n'''

srv6_data2 = '''                    case 1: l4.setSrv6_1_Protocol(Srv6_1_Protocol.read(bb));
                        break;\n'''

srv6_data3 = '''                    default:
                        throw new UnsupportedOperationException("L4Layer type is unsupported");
                }
                break;\n'''

start = '''/*
 * Copyright 2014-present Open Networking Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


package org.onosproject.net.packet.mof;


import org.onlab.packet.BasePacket;
import java.util.Objects;
import static com.google.common.base.Preconditions.checkNotNull;
import io.netty.buffer.ByteBuf;
import io.netty.buffer.Unpooled;
import org.onosproject.net.flow.instructions.protocol.Specific_Protocol;\n\n'''

data_import = 'import org.onosproject.net.flow.instructions.protocol.Mac_Protocol;\n'

data0 = '\npublic class MOFL3Layer extends BasePacket {\n'

data1 = '    private Mac_Protocol mac;\n'

mid1 = '''    private int LEN = 0;
    private int nextType = -1;
    
    /**
     * By default, set MOFL3Layer to untagged.
    */
    public MOFL3Layer() {
        super();
    }

    public int Length(){
        return LEN;
    }\n\n'''

data2 = '''    public boolean isMac_Protocol(){
        return mac != null;
    }\n'''

data3 = '''\n    public static MOFL3Layer parse(int type, byte[] data, int offset){
        MOFL3Layer l3 = new MOFL3Layer();
        int length = data.length - offset;
        if(Specific_Protocol.getInstance().contains(type)){
            Specific_Protocol.getInstance().parse_specific_protocolL3(type, Unpooled.wrappedBuffer(data, offset, length), l3);
        }else{
            switch (type) {\n'''

data4 = '''            case 0x0800:
                l3.setMac_Protocol(Mac_Protocol.read(Unpooled.wrappedBuffer(data, offset, length)));
                break;\n'''

data5 = '''            default:
                throw new UnsupportedOperationException("L3Layer type 0x" + Integer.toHexString(type) + " is unsupported");
            }
        }

        return l3;
    }

    public int getNextType(){
        return nextType;
    }\n\n'''

data6 = '''    public void setMac_Protocol(Mac_Protocol mac){
        this.mac = mac;
        this.nextType = (int)mac.mac_type.value();
        this.LEN = Mac_Protocol.LEN;
    }
 
    public Mac_Protocol getMac_Protocol(){
        checkNotNull(mac, "get Mac_Protocol value is null");
        return mac;
    }\n\n'''

mid2 = '''    @Override
    public byte[] serialize() {
        return null;
    }

    /*
    * (non-Javadoc)
    *
    * @see java.lang.Object#hashCode()
    */
    @Override
    public int hashCode() {\n'''

data7 = '''        if(mac != null){
            return mac.hashCode();
        }\n'''

mid3 = '''\n        return super.hashCode();
    }

    /*
    * (non-Javadoc)
    *
    * @see java.lang.Object#equals(java.lang.Object)
    */
    @Override
    public boolean equals(final Object obj) {\n'''

data8 = '''        if(mac != null){
            return Objects.equals(mac, obj);
        }\n'''

end ='''\n        return super.equals(obj);
    }
}\n'''

def create_MOFlayer(Layeri, lay, last_lay):
    with open('../header_describe/main.yml', 'r', encoding='utf-8') as f:
        result = yaml.load(f.read(), Loader=yaml.FullLoader)
        
    # print(Layeri, lay, last_lay)
    sp_datar = sp_data.replace('3', lay)
    for name in Layeri:
        with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
            Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
    ll_result = []
    for name in last_lay:
        with open('../header_describe/'+name+'.yml', 'r', encoding='utf-8') as f:
            # ll_result = yaml.load(f.read(), Loader=yaml.FullLoader)
            ll_result.append(yaml.load(f.read(), Loader=yaml.FullLoader))
    type = {}
    for pro in ll_result:
        for key, value in pro.items():
            if value['Type'] is not None:
                type.update(value['Type'])
    outputpath = './javafile/mof/MOFL' + lay + 'Layer.java'
    with open(outputpath, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(start)
    for protocol in Layeri:
        if (result['Specific'] is None) or (protocol not in result['Specific'].keys()):
            data_importr = data_import.replace('Mac', protocol.title())
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data_importr)
        else:
            sp_datar = sp_datar.replace('''        throw new UnsupportedOperationException("L''' + str(lay) + '''Layer has no specific protocol yet");
    }\n\n''', sp_data1)
            if protocol == 'srv6':
                sp_datar = sp_datar + srv6_data1
                for key, val in Li_result.items():
                    for key_, val_ in val.items():
                        if key_ == "srv6_segmentlist": 
                            srv6_segl = int(val_['size']) % 128
                            for inx in range(0, srv6_segl):
                                data_importr = data_import.replace('Mac', protocol.title() + '_' + str(inx + 1))
                                with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                    file_obj.write(data_importr)
                                srv6_data2r = srv6_data2.replace('1', str(inx + 1))
                                sp_datar = sp_datar + srv6_data2r
                sp_datar = sp_datar + srv6_data3
            else:
                sp_data2r = sp_data2.replace('3', lay) 
                sp_data2r = sp_data2r.replace('0x12', result['Specific'][protocol])
                sp_datar = sp_datar + sp_data2r
            sp_datar = sp_datar + sp_data3
    data0r = data0.replace('3', lay)                            
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(data0r)
    for protocol in Layeri:
        if protocol != 'srv6':
            data1r = data1.replace('Mac', protocol.title())
            data1r = data1r.replace('mac', protocol)
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data1r)
        else:
            for key, val in Li_result.items():
                for key_, val_ in val.items():
                    if key_ == "srv6_segmentlist": 
                        srv6_segl = int(val_['size']) % 128
                        for inx in range(0, srv6_segl):
                            data1r = data1.replace('Mac', protocol.title() + '_' + str(inx + 1))
                            data1r = data1r.replace('mac', protocol + '_' + str(inx + 1))
                            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                file_obj.write(data1r)      
    mid1r = mid1.replace('3', lay)
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid1r)
    for protocol in Layeri:
        if protocol != 'srv6':
            data2r = data2.replace('Mac', protocol.title())
            data2r = data2r.replace('mac', protocol)
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data2r)
        else:
            for key, val in Li_result.items():
                for key_, val_ in val.items():
                    if key_ == "srv6_segmentlist": 
                        srv6_segl = int(val_['size']) % 128
                        for inx in range(0, srv6_segl):
                            data2r = data2.replace('Mac', protocol.title() + '_' + str(inx + 1))
                            data2r = data2r.replace('mac', protocol + '_' + str(inx + 1))
                            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                file_obj.write(data2r)
    data3r = data3.replace('3', lay)                            
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(data3r)
    for protocol in Layeri:
        type_value = '0x00'
        if type is not None:
            # print(protocol, type)
            for type_key in type.keys():
                if protocol in type_key:
                    type_value = type[type_key]
        if (result['Specific'] is not None) and (protocol not in result['Specific'].keys()):
            data4r = data4.replace('3', lay)
            data4r = data4r.replace('Mac', protocol.title())
            data4r = data4r.replace('0x0800', type_value)
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data4r)
        # else:
        #     for key, val in Li_result.items():
        #         for key_, val_ in val.items():
        #             if key_ == "srv6_segmentlist": 
        #                 srv6_segl = int(val_['size']) % 128
        #                 for inx in range(0, srv6_segl):
        #                     data4r = data4.replace('3', lay)
        #                     data4r = data4r.replace('Mac', protocol.title() + '_' + str(inx + 1))
        #                     data4r = data4r.replace('0x0800', type_value)
        #                     with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        #                         file_obj.write(data4r)  
    data5r = data5.replace('3', lay)                            
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(data5r)
    for protocol in Layeri:
        if protocol != 'srv6':
            with open('../header_describe/'+protocol+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
            data6r = data6.replace('Mac', protocol.title())
            data6r = data6r.replace('mac', protocol)
            if Li_result[protocol]['Type'] == None:
                # print(protocol, '(int)' + protocol + '.' + protocol + '_type.value();')
                data6r = data6r.replace('(int)' + protocol + '.' + protocol + '_type.value()', str(-1))
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data6r)
        else:
            with open('../header_describe/'+protocol+'.yml', 'r', encoding='utf-8') as f:
                Li_result = yaml.load(f.read(), Loader=yaml.FullLoader)
            for key, val in Li_result.items():
                for key_, val_ in val.items():
                    if key_ == "srv6_segmentlist": 
                        srv6_segl = int(val_['size']) % 128
                        for inx in range(0, srv6_segl):
                            data6r = data6.replace('mac_type', protocol + '_type')
                            data6r = data6r.replace('Mac', protocol.title() + '_' + str(inx + 1))
                            data6r = data6r.replace('mac', protocol + '_' + str(inx + 1))
                            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                file_obj.write(data6r)
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid2)
    for protocol in Layeri:
        if protocol != 'srv6':
            data7r = data7.replace('mac', protocol)
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data7r)
        else:
            for key, val in Li_result.items():
                for key_, val_ in val.items():
                    if key_ == "srv6_segmentlist": 
                        srv6_segl = int(val_['size']) % 128
                        for inx in range(0, srv6_segl):
                            data7r = data7.replace('mac', protocol + '_' + str(inx + 1))
                            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                file_obj.write(data7r)
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid3)
    for protocol in Layeri:
        if protocol != 'srv6':
            data8r = data8.replace('mac', protocol)
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data8r)
        else:
            for key, val in Li_result.items():
                for key_, val_ in val.items():
                    if key_ == "srv6_segmentlist": 
                        srv6_segl = int(val_['size']) % 128
                        for inx in range(0, srv6_segl):
                            data8r = data8.replace('mac', protocol + '_' + str(inx + 1))
                            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                file_obj.write(data8r)
    with open('./javafile/instructions/protocol/Specific_Protocol.java', mode='a', encoding='utf-8') as file_obj:
        file_obj.write(sp_datar)
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(end)