import os

start = '''package org.onosproject.net.flow.instructions.protocol;

import java.util.Objects;
import io.netty.buffer.ByteBuf;
import org.onosproject.net.flow.criteria.Criterion;\n\n'''

data_import = 'import org.onosproject.net.flow.criteria.XxCriterion;\n'

data_class = '\npublic class Mac_Protocol implements Protocol {\n'

data1 = '    public Mac_DstCriterion mac_dst;\n'

data2_0 = '    public static int LEN ='
data2_1 = ' Mac_DstCriterion.LEN +'

data3_1 = '\n    public Mac_Protocol('
data3_2 = 'Mac_DstCriterion mac_dst, '

data4 = '        this.mac_dst = mac_dst;\n'

data5_0 = '''    }

    @Override
    public void write(ByteBuf bb){\n'''
data5_1 = '        mac_dst.write(bb);\n'

data6_0 = '''        bb.writeZero(44);
    }
  
    @Override
    public void writeMask(ByteBuf bb){\n'''
data6_1 = '        mac_dst.writeMask(bb);\n'

data7_1 = '''        bb.writeZero(44);
    }
  
    public static Mac_Protocol read(ByteBuf bb){\n'''
data7_2 = '''        Mac_DstCriterion mac_dst = new Mac_DstCriterion.Builder()
                                                .setValid(true)
                                                .readData(bb)
                                                .build();\n\n'''
                                            
data8_1 = '''        bb.skipBytes(44);
        return new Mac_Protocol('''
data8_2 = 'mac_dst, '

data9_0 = '''    }

    @Override
    public String toString() {\n'''
data9_1 = '        return "Mac_Protocol{ "'
data9_2 = ' + mac_dst + ", "'

data10_0 ='''    }

    @Override
    public int hashCode() {
        return Objects.hash('''
data10_1 = 'mac_dst, '

data11_1 = '''    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj instanceof Mac_Protocol) {
            Mac_Protocol that = (Mac_Protocol) obj;
            return'''
data11_2 = ' Objects.equals(mac_dst, that.mac_dst) &&'

data12_1 = '''        }
        return false;
    }
    public static Mac_Protocol readWithMask(ByteBuf bb){\n'''

data12_2 = '        Mac_DstCriterion.Builder b1 = new Mac_DstCriterion.Builder();\n'

data13_1 = '        b1.readData(bb);\n'
data13_2 = '        bb.skipBytes(44);\n\n'

data14_1 = '        b1.readMask(bb);\n'
data14_2 = '        bb.skipBytes(44);\n\n'

data15_1 = '        return new Mac_Protocol('
data15_2 = 'b1.build(), '

data_end = '''    }
  
}'''

def create_Protocol_class(Li_result, max_len):
    index = 1
    length = 0
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                if key_ == "srv6_segmentlist":
                    length += val_['size'] % 128 * 128
                else:
                    length += val_['size']
    length //= 8
    # print(length)
    for key, val in Li_result.items():
        if key != 'srv6':
            outputpath0 = './javafile/instructions/protocol/' + key.title() + '_Protocol.java'
            with open(outputpath0, mode='w', encoding='utf-8') as file_obj:
                file_obj.write(start)
            outputpath = ['./javafile/instructions/protocol/']
            for i in range(1, 16):
                outputpath.append(outputpath[0] + key.title() + str(i))
            # print(outputpath)
            with open(outputpath[2], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data2_0)
            data3_1r = data3_1.replace('Mac', key.title())
            with open(outputpath[3], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data3_1r)
            with open(outputpath[5], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data5_0)
            # if length != max_len:
            if length % 8 == 0:
                data6_0r = data6_0.replace('bb.writeZero(44);', '')
            else:
                data6_0r = data6_0.replace('44', str(8 - length % 8))
            # else:
            #     data6_0r = data6_0.replace('bb.writeZero(44);', '')
            with open(outputpath[6], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data6_0r)
            data7_1r = data7_1.replace('Mac', key.title())
            # if length != max_len:
            #     data7_1r = data7_1r.replace('44', str(max_len - length))
            # else:
            #     data7_1r = data7_1r.replace('bb.writeZero(44);', '')
            if length % 8 == 0:
                data7_1r = data7_1r.replace('bb.writeZero(44);', '')
            else:
                data7_1r = data7_1r.replace('44', str(8 - length % 8))
            with open(outputpath[7], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data7_1r)
            data8_1r = data8_1.replace('Mac', key.title())
            # if length != max_len:
            #     data8_1r = data8_1r.replace('44', str(max_len - length))
            # else:
            #     data8_1r = data8_1r.replace('bb.skipBytes(44);', '')
            if length % 8 == 0:
                data8_1r = data8_1r.replace('bb.skipBytes(44);', '')
            else:
                data8_1r = data8_1r.replace('44', str(8 - length % 8))
            with open(outputpath[8], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data8_1r)
            with open(outputpath[9], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data9_0)    
            data9_1r = data9_1.replace('Mac', key.title())
            with open(outputpath[9], mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data9_1r)
            with open(outputpath[10], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data10_0)
            data11_1r = data11_1.replace('Mac', key.title())
            with open(outputpath[11], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data11_1r)
            data12_1r = data12_1.replace('Mac', key.title())
            with open(outputpath[12], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data12_1r)
            data15_1r = data15_1.replace('Mac', key.title())
            with open(outputpath[15], mode='w', encoding='utf-8') as file_obj:
                file_obj.write(data15_1r)
            for key_, val_ in val.items():
                if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                    data_importr = data_import.replace('Xx', key_.title())
                    with open(outputpath0, mode='a', encoding='utf-8') as file_obj:
                        file_obj.write(data_importr)
                    data1r = data1.replace('Mac_Dst', key_.title())
                    data1r = data1r.replace('mac_dst', key_)
                    write(outputpath[1], data1r)
                    data2_1r = data2_1.replace('Mac_Dst', key_.title())
                    write(outputpath[2], data2_1r)
                    data3_2r = data3_2.replace('Mac_Dst', key_.title())
                    data3_2r = data3_2r.replace('mac_dst', key_)
                    write(outputpath[3], data3_2r)
                    data4r = data4.replace('mac_dst', key_)
                    write(outputpath[4], data4r)
                    data5_1r = data5_1.replace('mac_dst', key_)
                    write(outputpath[5], data5_1r)
                    data6_1r = data6_1.replace('mac_dst', key_)
                    write(outputpath[6], data6_1r)
                    data7_2r = data7_2.replace('Mac_Dst', key_.title())
                    data7_2r = data7_2r.replace('mac_dst', key_)
                    write(outputpath[7], data7_2r)
                    data8_2r = data8_2.replace('mac_dst', key_)
                    write(outputpath[8], data8_2r)
                    data9_2r = data9_2.replace('mac_dst', key_)
                    write(outputpath[9], data9_2r)
                    data10_1r = data10_1.replace('mac_dst', key_)
                    write(outputpath[10], data10_1r)
                    data11_2r = data11_2.replace('mac_dst', key_)
                    write(outputpath[11], data11_2r)
                    data12_2r = data12_2.replace('1', str(index))
                    data12_2r = data12_2r.replace('Mac_Dst', key_.title())
                    write(outputpath[12], data12_2r)
                    data13_1r = data13_1.replace('1', str(index))
                    write(outputpath[13], data13_1r)
                    data14_1r = data14_1.replace('1', str(index))
                    write(outputpath[14], data14_1r)
                    data15_2r = data15_2.replace('1', str(index))
                    write(outputpath[15], data15_2r)
                    index += 1
            # if length != max_len:
            #     data13_2r = data13_2.replace('44', str(max_len - length))
            # else:
            #     data13_2r = data13_2.replace('bb.skipBytes(44);', '')
            
            if length % 8 == 0:
                data13_2r = data13_2.replace('bb.skipBytes(44);', '')
            else:
                data13_2r = data13_2.replace('44', str(8 - length % 8))
            write(outputpath[13], data13_2r)
            # if length != max_len:
            #     data14_2r = data14_2.replace('44', str(max_len - length))
            # else:
            #     data14_2r = data14_2.replace('bb.skipBytes(44);', '')

            if length % 8 == 0:
                data14_2r = data14_2.replace('bb.skipBytes(44);', '')
            else:
                data14_2r = data14_2.replace('44', str(8 - length % 8))
            write(outputpath[14], data14_2r)
            # ---------------
            with open(outputpath[2], 'r') as file:
                lines = file.readlines()
            last_line = lines[-1].rstrip('\n')
            updated_last_line = last_line[:-2] + ';\n'
            lines[-1] = updated_last_line
            with open(outputpath[2], 'w') as file:
                file.writelines(lines)

            with open(outputpath[3], 'r') as file:
                lines = file.readlines()
            last_line = lines[-1].rstrip('\n')
            updated_last_line = last_line[:-2] + '){\n'
            lines[-1] = updated_last_line
            with open(outputpath[3], 'w') as file:
                file.writelines(lines)

            with open(outputpath[8], 'r') as file:
                lines = file.readlines()
            last_line = lines[-1].rstrip('\n')
            updated_last_line = last_line[:-2] + ');\n'
            lines[-1] = updated_last_line
            with open(outputpath[8], 'w') as file:
                file.writelines(lines)

            with open(outputpath[9], 'r') as file:
                lines = file.readlines()
            last_line = lines[-1].rstrip('\n')
            updated_last_line = last_line[:-3] + ' }";\n'
            lines[-1] = updated_last_line
            with open(outputpath[9], 'w') as file:
                file.writelines(lines)
            
            with open(outputpath[10], 'r') as file:
                lines = file.readlines()
            last_line = lines[-1].rstrip('\n')
            updated_last_line = last_line[:-2] + ');\n'
            lines[-1] = updated_last_line
            with open(outputpath[10], 'w') as file:
                file.writelines(lines)

            with open(outputpath[11], 'r') as file:
                lines = file.readlines()
            last_line = lines[-1].rstrip('\n')
            updated_last_line = last_line[:-3] + ';\n'
            lines[-1] = updated_last_line
            with open(outputpath[11], 'w') as file:
                file.writelines(lines)
            
            with open(outputpath[15], 'r') as file:
                lines = file.readlines()
            last_line = lines[-1].rstrip('\n')
            updated_last_line = last_line[:-2] + ');\n'
            lines[-1] = updated_last_line
            with open(outputpath[15], 'w') as file:
                file.writelines(lines)

            write(outputpath[15], data_end)
            data_classr = data_class.replace('Mac', key.title())
            with open(outputpath0, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data_classr)
                      
            del outputpath[0]
            with open(outputpath0, 'a') as outfile:
                for file_name in outputpath:
                    with open(file_name, 'r') as infile:
                        outfile.write(infile.read())
            for i in range(0, 15):
                try:
                    os.remove(outputpath[i])
                except OSError as e:
                    a = 1
        else:
            for key_, val_ in val.items():
                if key_ == "srv6_segmentlist": 
                    srv6_segl = int(val_['size']) % 128
                    for inx in range(0, srv6_segl):
                        index = 1
                        length = 8 + (inx + 1) * 16
                        outputpath0 = './javafile/instructions/protocol/' + key.title() + '_' + str(inx + 1) + '_Protocol.java'
                        with open(outputpath0, mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(start)
                        outputpath = ['./javafile/instructions/protocol/']
                        for i in range(1, 16):
                            outputpath.append(outputpath[0] + key.title() + '_' + str(inx + 1) + str(i))
                        with open(outputpath[2], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data2_0)
                        data3_1r = data3_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        with open(outputpath[3], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data3_1r)
                        with open(outputpath[5], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data5_0)
                        # if length != max_len:
                        #     data6_0r = data6_0.replace('44', str(max_len - length))
                        # else:
                        #     data6_0r = data6_0.replace('bb.writeZero(44);', '')

                        if length % 8 == 0:
                            data6_0r = data6_0.replace('bb.writeZero(44);', '')
                        else:
                            data6_0r = data6_0.replace('44', str(8 - length % 8))
                        with open(outputpath[6], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data6_0r)
                        data7_1r = data7_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        # if length != max_len:
                        #     data7_1r = data7_1r.replace('44', str(max_len - length))
                        # else:
                        #     data7_1r = data7_1r.replace('bb.writeZero(44);', '')

                        if length % 8 == 0:
                            data7_1r = data7_1r.replace('bb.writeZero(44);', '')
                        else:
                            data7_1r = data7_1r.replace('44', str(8 - length % 8))
                        with open(outputpath[7], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data7_1r)
                        data8_1r = data8_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        # data8_1r = data8_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        # if length != max_len:
                        #     data8_1r = data8_1r.replace('44', str(max_len - length))
                        # else:
                        #     data8_1r = data8_1r.replace('bb.skipBytes(44);', '')

                        if length % 8 == 0:
                            data8_1r = data8_1r.replace('bb.skipBytes(44);', '')
                        else:
                            data8_1r = data8_1r.replace('44', str(8 - length % 8))
                        with open(outputpath[8], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data8_1r)
                        with open(outputpath[9], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data9_0)    
                        data9_1r = data9_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        with open(outputpath[9], mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data9_1r)
                        with open(outputpath[10], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data10_0)
                        data11_1r = data11_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        with open(outputpath[11], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data11_1r)
                        data12_1r = data12_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        with open(outputpath[12], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data12_1r)
                        data15_1r = data15_1.replace('Mac', key.title() + '_' + str(inx + 1))
                        with open(outputpath[15], mode='w', encoding='utf-8') as file_obj:
                            file_obj.write(data15_1r)
                        for key_, val_ in val.items():
                            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                                if key_ != "srv6_segmentlist":
                                    data_importr = data_import.replace('Xx', key_.title())
                                    with open(outputpath0, mode='a', encoding='utf-8') as file_obj:
                                        file_obj.write(data_importr)
                                    data1r = data1.replace('Mac_Dst', key_.title())
                                    data1r = data1r.replace('mac_dst', key_)
                                    write(outputpath[1], data1r)
                                    data2_1r = data2_1.replace('Mac_Dst', key_.title())
                                    write(outputpath[2], data2_1r)
                                    data3_2r = data3_2.replace('Mac_Dst', key_.title())
                                    data3_2r = data3_2r.replace('mac_dst', key_)
                                    write(outputpath[3], data3_2r)
                                    data4r = data4.replace('mac_dst', key_)
                                    write(outputpath[4], data4r)
                                    data5_1r = data5_1.replace('mac_dst', key_)
                                    write(outputpath[5], data5_1r)
                                    data6_1r = data6_1.replace('mac_dst', key_)
                                    write(outputpath[6], data6_1r)
                                    data7_2r = data7_2.replace('Mac_Dst', key_.title())
                                    data7_2r = data7_2r.replace('mac_dst', key_)
                                    write(outputpath[7], data7_2r)
                                    data8_2r = data8_2.replace('mac_dst', key_)
                                    write(outputpath[8], data8_2r)
                                    data9_2r = data9_2.replace('mac_dst', key_)
                                    write(outputpath[9], data9_2r)
                                    data10_1r = data10_1.replace('mac_dst', key_)
                                    write(outputpath[10], data10_1r)
                                    data11_2r = data11_2.replace('mac_dst', key_)
                                    write(outputpath[11], data11_2r)
                                    data12_2r = data12_2.replace('1', str(index))
                                    data12_2r = data12_2r.replace('Mac_Dst', key_.title())
                                    write(outputpath[12], data12_2r)
                                    data13_1r = data13_1.replace('1', str(index))
                                    write(outputpath[13], data13_1r)
                                    data14_1r = data14_1.replace('1', str(index))
                                    write(outputpath[14], data14_1r)
                                    data15_2r = data15_2.replace('1', str(index))
                                    write(outputpath[15], data15_2r)
                                    index += 1
                                else:
                                    for j in range(0, inx + 1):
                                        # print(key_.title() + str(j + 1))
                                        data_importr = data_import.replace('Xx', key_.title() + str(j + 1))
                                        with open(outputpath0, mode='a', encoding='utf-8') as file_obj:
                                            file_obj.write(data_importr)
                                        data1r = data1.replace('Mac_Dst', key_.title() + str(j + 1))
                                        data1r = data1r.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[1], data1r)
                                        data2_1r = data2_1.replace('Mac_Dst', key_.title() + str(j + 1))
                                        write(outputpath[2], data2_1r)
                                        data3_2r = data3_2.replace('Mac_Dst', key_.title() + str(j + 1))
                                        data3_2r = data3_2r.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[3], data3_2r)
                                        data4r = data4.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[4], data4r)
                                        data5_1r = data5_1.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[5], data5_1r)
                                        data6_1r = data6_1.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[6], data6_1r)
                                        data7_2r = data7_2.replace('Mac_Dst', key_.title() + str(j + 1))
                                        data7_2r = data7_2r.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[7], data7_2r)
                                        data8_2r = data8_2.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[8], data8_2r)
                                        data9_2r = data9_2.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[9], data9_2r)
                                        data10_1r = data10_1.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[10], data10_1r)
                                        data11_2r = data11_2.replace('mac_dst', key_ + str(j + 1))
                                        write(outputpath[11], data11_2r)
                                        data12_2r = data12_2.replace('1', str(index))
                                        data12_2r = data12_2r.replace('Mac_Dst', key_.title() + str(j + 1))
                                        write(outputpath[12], data12_2r)
                                        data13_1r = data13_1.replace('1', str(index))
                                        write(outputpath[13], data13_1r)
                                        data14_1r = data14_1.replace('1', str(index))
                                        write(outputpath[14], data14_1r)
                                        data15_2r = data15_2.replace('1', str(index))
                                        write(outputpath[15], data15_2r)
                                        index += 1
                        # if length != max_len:
                        #     data13_2r = data13_2.replace('44', str(max_len - length))
                        # else:
                        #     data13_2r = data13_2.replace('bb.skipBytes(44);', '')
                        if length % 8 == 0:
                            data13_2r = data13_2.replace('bb.skipBytes(44);', '')
                        else:
                            data13_2r = data13_2.replace('44', str(8 - length % 8))
                        write(outputpath[13], data13_2r)
                        # if length != max_len:
                        #     data14_2r = data14_2.replace('44', str(max_len - length))
                        # else:
                        #     data14_2r = data14_2.replace('bb.skipBytes(44);', '')

                        if length % 8 == 0:
                            data14_2r = data14_2.replace('bb.skipBytes(44);', '')
                        else:
                            data14_2r = data14_2.replace('44', str(8 - length % 8))
                        write(outputpath[14], data14_2r)

                        with open(outputpath[2], 'r') as file:
                            lines = file.readlines()
                        last_line = lines[-1].rstrip('\n')
                        updated_last_line = last_line[:-2] + ';\n'
                        lines[-1] = updated_last_line
                        with open(outputpath[2], 'w') as file:
                            file.writelines(lines)
                        with open(outputpath[3], 'r') as file:
                            lines = file.readlines()
                        last_line = lines[-1].rstrip('\n')
                        updated_last_line = last_line[:-2] + '){\n'
                        lines[-1] = updated_last_line
                        with open(outputpath[3], 'w') as file:
                            file.writelines(lines)

                        with open(outputpath[8], 'r') as file:
                            lines = file.readlines()
                        last_line = lines[-1].rstrip('\n')
                        updated_last_line = last_line[:-2] + ');\n'
                        lines[-1] = updated_last_line
                        with open(outputpath[8], 'w') as file:
                            file.writelines(lines)

                        with open(outputpath[9], 'r') as file:
                            lines = file.readlines()
                        last_line = lines[-1].rstrip('\n')
                        updated_last_line = last_line[:-3] + ' }";\n'
                        lines[-1] = updated_last_line
                        with open(outputpath[9], 'w') as file:
                            file.writelines(lines)
                        
                        with open(outputpath[10], 'r') as file:
                            lines = file.readlines()
                        last_line = lines[-1].rstrip('\n')
                        updated_last_line = last_line[:-2] + ');\n'
                        lines[-1] = updated_last_line
                        with open(outputpath[10], 'w') as file:
                            file.writelines(lines)

                        with open(outputpath[11], 'r') as file:
                            lines = file.readlines()
                        last_line = lines[-1].rstrip('\n')
                        updated_last_line = last_line[:-3] + ';\n'
                        lines[-1] = updated_last_line
                        with open(outputpath[11], 'w') as file:
                            file.writelines(lines)
                        
                        with open(outputpath[15], 'r') as file:
                            lines = file.readlines()
                        last_line = lines[-1].rstrip('\n')
                        updated_last_line = last_line[:-2] + ');\n'
                        lines[-1] = updated_last_line
                        with open(outputpath[15], 'w') as file:
                            file.writelines(lines)
                        
                        write(outputpath[15], data_end)
                        data_classr = data_class.replace('Mac', key.title() + '_' + str(inx + 1))
                        with open(outputpath0, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data_classr)
                                
                        del outputpath[0]
                        with open(outputpath0, 'a') as outfile:
                            for file_name in outputpath:
                                with open(file_name, 'r') as infile:
                                    outfile.write(infile.read())
                        for i in range(0, 15):
                            try:
                                os.remove(outputpath[i])
                            except OSError as e:
                                a = 1
                                

def write(output_file, data):
    with open(output_file, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(data)

