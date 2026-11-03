outputpath = './javafile/parser/CriterionParser.java'

general_size = [8, 16, 32]

start = '''package org.onosproject.net.flow.criteria.parser;

import org.onosproject.net.flow.criteria.Criterion.Type;;

public final class CriterionParser {
    
    final static String PREFIX = "0x";
    //parser 1, 2, 4bytes 
    public static String BasicParser(long data, long mask, Type type) {
        switch(type){
        case IN_PORT:
            return data + "";\n'''

end = '''        default:
            return PREFIX + Long.toHexString(data) + "/" + PREFIX + Long.toHexString(mask).toUpperCase();
        }
    }

    // others 
    public static String ComplexParser(byte[] data, byte[] mask, Type type) {
        switch(type){\n'''

end1 = '''        default:
            return PREFIX + encodeHexStringHelper(data) + "/" + PREFIX + encodeHexStringHelper(mask);
        }
    }

    /*****************************************************************************
     * Parser Helper Function
     *****************************************************************************/

    /**
     * Parser hepler function
     * @input  byte[] data
     * @return Hex format String of data
     */
    public static String encodeHexStringHelper(byte[] data) {
        StringBuilder sb = new StringBuilder();
        for (byte b : data) {
            sb.append(String.format("%02X", b));
        }
        return sb.toString();
    }

    /**
     * Parser hepler function
     * @input  byte[] data : ipv6 Address
     *         byte[] mask : ipv6 Mask Address
     * @return customer-friendly IPV6 Fromat String
     */
    public static String encodeIPV6StringHelper(byte[] data, byte[] mask) {
        String[] result = new String[2];

        // Convert IP bytes to IPv6 string
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < 16; i += 2) {
            sb.append(String.format("%02X%02X", data[i], data[i + 1]));
            if (i < 14) {
                sb.append(":");
            }
        }
        result[0] = sb.toString();
    
        // Convert mask bytes to prefix length
        int prefixLength = 0;
        for (byte b : mask) {
            for (int i = 7; i >= 0; i--) {
                if ((b & (1 << i)) != 0) {
                    prefixLength++;
                } else {
                    break;
                }
            }
        }
        result[1] = "/" + prefixLength;
    
        return result[0] + result[1];
    }

    /**
     * Parser hepler function
     * @input  int data : ipv4 Address
     *         int mask : ipv4 Mask Address
     * @return customer-friendly IPV4 Fromat String
     */
    public static String encodeIPV4StringHelper(long data, long mask) {
        String[] result = new String[2];
        result[0] = String.format("%d.%d.%d.%d", (data >> 24) & 0xFF, (data >> 16) & 0xFF, (data >> 8) & 0xFF, data & 0xFF);

        int bits = 0;
        for (int i = 31; i >= 0; i--) {
            if ((mask & (1L << i)) != 0) {
                bits++;
            } else {
                break;
            }
        }
        result[1] = "/" + bits;

        return result[0] + result[1];
    }

    /**
     * Parser hepler function
     * @input  byte[] data : MAC Address
     *         byte[] mask : MAC Mask Address
     * @return customer-friendly MAC Fromat String
     */
    public static String encodeMACStringHelper(byte[] data, byte[] mask) {
        String address = String.format("%02X:%02X:%02X:%02X:%02X:%02X", 
                            data[0], data[1], data[2], data[3], data[4], data[5]);

        int bits = 0;
        for (byte b : mask) {
            for (int i = 7; i >= 0; i--) {
                if ((b & (1 << i)) != 0) {
                    bits++;
                } else {
                    break;
                }
            }
        }

        return address + "/" + bits;
    }
}\n\n'''

data = '        case MAC_DST:\n'


def create_CriterionParser_start():
    with open(outputpath, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(start)


def create_CriterionParser(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                if key_ != "srv6_segmentlist" and val_['size'] in general_size:
                    data_r = data.replace('MAC_DST', key_.upper())
                    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                        file_obj.write(data_r)


def create_CriterionParser1(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                if key_ != "srv6_segmentlist" and val_['size'] not in general_size:
                        data_r = data.replace('MAC_DST', key_.upper())
                        with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data_r)
                elif key_ == "srv6_segmentlist":
                    srv6_segl = int(val_['size']) % 128
                    while srv6_segl > 0: 
                        data_r = data.replace('MAC_DST', key_.upper() + str(srv6_segl))
                        with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data_r)
                        srv6_segl -= 1


def create_CriterionParser_end():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(end)


def create_CriterionParser_end1():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(end1)