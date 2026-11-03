import yaml
import java_create_CriterionCodec

outputpath1 = './javafile/TrafficSelector.java'
fo1 = open(outputpath1, 'w', encoding='utf-8')

fo_DefaultTrafficSelector_import = open('./javafile/DefaultTrafficSelector_import.java', 'w', encoding='utf-8')
fo_TrafficSelector_import = open('./javafile/TrafficSelector_import.java', 'w', encoding='utf-8')
fo_criteria_import = open('./javafile/criteria/criteria_import.java', 'w', encoding='utf-8')

bits, flow_len =  560*8, 736*8
general_size = [8, 16, 32]
size_trans = {8: 'Byte', 16: 'short', 32: 'int'}

public_class = '''package org.onlab.packet;

import com.google.common.collect.ImmutableSet;
import java.util.Arrays;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class dd {
  
  	public static final int LEN = yy;
    private byte[] xx = new byte[LEN];

    public dd(final byte[] field) {
        this.xx = Arrays.copyOf(field, LEN);
    }
  
    public static dd valueOf(final byte[] field) {
        if (field.length > LEN) {
            throw new IllegalArgumentException("dd valueOf out of the bound should " + 
                                                LEN + "get " + field.length);
        }

        return new dd(field);
    }
  
    public int length() {
        return this.xx.length;
    }
  
    public byte[] toBytes() {
        return Arrays.copyOf(this.xx, this.xx.length);
    }
  	
  	@Override
    public boolean equals(final Object o) {
        if (o == this) {
            return true;
        }

        if (!(o instanceof dd)) {
            return false;
        }

        final dd other = (dd) o;
        return Arrays.equals(this.xx, other.xx);
    }

  	@Override
    public int hashCode() {
        return (int)xx[0];
    }
		
    @Override
    public String toString() {
      return encodeHexString();
    }
    
    
    public String encodeHexString() {
        StringBuilder sb = new StringBuilder();
        for (byte b : this.xx) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }
}
'''

#---------------------------TraficSelector------------------------------------#
def create_public_interface_TrafficSelector(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                import_str = "import org.onosproject.net.flow.criteria." + key_.title() +'Criterion;\n'
                java_create_CriterionCodec.add_import(import_str)
                fo_DefaultTrafficSelector_import.write("import org.onosproject.net.flow.criteria." + key_.title() +'Criterion;\n')
                if (val_['size'] not in general_size):
                    fo1.write("\n        Builder select" + key_.title() +
                              "(" + key_.title() + ' ' + key_ + ");\n")
                    fo1.write("        Builder select" + key_.title() + "(" + key_.title() + ' ' + key_ +
                              ", " + key_.title() + " mask);\n")
                    outputpath = "./javafile/packet/" + key_.title() + ".java"
                    # print(outputpath)
                    fo = open(outputpath, 'w', encoding='utf-8')
                    public_class_r = public_class.replace('xx', key_)
                    public_class_r = public_class_r.replace('dd', key_.title())
                    public_class_r = public_class_r.replace(
                        'yy', str(int(val_['size'])//8))
                    fo.write(public_class_r)
                    fo_DefaultTrafficSelector_import.write("import org.onlab.packet." + key_.title() +';\n')
                    fo_TrafficSelector_import.write("import org.onlab.packet." + key_.title() +';\n')
                    fo_criteria_import.write("import org.onlab.packet." + key_.title() +';\n')
                else:
                    fo1.write("\n        Builder select" + key_.title() + "(" +
                              'long' + ' ' + key_ + ");\n")
                    fo1.write("        Builder select" + key_.title() + "(" + 'long' + ' ' + key_ +
                              ", " + 'long' + " mask);\n")


#-------------------------------Criteria-------------------------------#
Criteria_path = './javafile/criteria/Criteria.java'
fo_Criteria = open(Criteria_path, 'w', encoding='utf-8')

field_public_final_class = '''    public static Criterion selectdd(yy xx) {
        return new ddCriterion(xx);
    }
  
    public static Criterion selectdd(yy xx, yy mask) {
        return new ddCriterion(xx, mask);
    }\n\n'''
    

def create_public_final_class_Criteria(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                field_public_final_class_r = field_public_final_class.replace('xx', key_)
                field_public_final_class_r = field_public_final_class_r.replace('dd', key_.title())
                if (val_['size'] not in general_size):
                    field_public_final_class_r = field_public_final_class_r.replace('yy', key_.title())
                    fo_Criteria.write(field_public_final_class_r)
                else:
                    field_public_final_class_r = field_public_final_class_r.replace('yy', 'long')
                    fo_Criteria.write(field_public_final_class_r)


#-----------------------------Criterion--------------------------------#
Criterion_path = './javafile/criteria/Criterion.java'
fo_Criterion = open(Criterion_path, 'w', encoding='utf-8')


def create_Criterion_java(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                fo_Criterion.write('        '+ key_.upper() + ',\n\n')
                
                

#-------------------------------DefaultTrafficSelector1-------------------------------#
DefaultTrafficSelector1_path = './javafile/DefaultTrafficSelector1.java'
fo_DefaultTrafficSelector1 = open(DefaultTrafficSelector1_path, 'w', encoding='utf-8')

field_public_class = '''        @Override
        public Builder selectzz(yy xx) {
            return add(Criteria.selectzz(xx));
        }
  
        @Override
        public Builder selectzz(yy xx, yy mask) {
            return add(Criteria.selectzz(xx, mask));
        }\n\n'''
    

def create_DefaultTrafficSelector1(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                field_public_class_r = field_public_class.replace('zz', key_.title())
                field_public_class_r = field_public_class_r.replace('xx', key_)
                if (val_['size'] not in general_size):
                    field_public_class_r = field_public_class_r.replace('yy', key_.title())
                    fo_DefaultTrafficSelector1.write(field_public_class_r)
                else:
                    field_public_class_r = field_public_class_r.replace('yy', 'long')
                    fo_DefaultTrafficSelector1.write(field_public_class_r)
                    
#-------------------------------DefaultTrafficSelector2-------------------------------#
DefaultTrafficSelector2_01_path = './javafile/DefaultTrafficSelector2_01.java'
DefaultTrafficSelector2_01_path = open(DefaultTrafficSelector2_01_path, 'w', encoding='utf-8')
DefaultTrafficSelector2_02_path = './javafile/DefaultTrafficSelector2_02.java'
DefaultTrafficSelector2_02_path = open(DefaultTrafficSelector2_02_path, 'w', encoding='utf-8')
DefaultTrafficSelector2_1_path = './javafile/DefaultTrafficSelector2_1.java'
DefaultTrafficSelector2_1_path = open(DefaultTrafficSelector2_1_path, 'w', encoding='utf-8')


wtireto_start = '''        return trafficSelector.build();
    }

    public static void writeStatsFlowRequestAllMatch(ByteBuf bb){

        bb.writeZero(8);
        bb.writeZero(200);
    
        bb.writeZero(8);
        bb.writeZero(200);

    }
    @Override
    public void writeTo(ByteBuf bb) {
        // log.info("DefaultTrafficSelector ready to write!!!");
        // mof
        // flow flow_mask pad

        // flow
        // before ETH_DST
        bb.writeZero(536);  \n\n'''

writeto_end = '''   bb.writeZero(2056);
  }\n'''
  
writeto_mid = '''        // flow_mask
        // pad_before inport
        bb.writeZero(536);\n\n'''

writeto_field_1 = '''        if(match.containsKey(Criterion.Type.xx)){
            match.get(Criterion.Type.xx).write(bb);
        }else{
            yyCriterion.writeZero(bb);
        }\n\n'''
      
writeto_field_2 = '''        if(match.containsKey(Criterion.Type.xx)){
            match.get(Criterion.Type.xx).writeMask(bb);
        }else{
            yyCriterion.writeZero(bb);
        }\n\n'''

data01 = '''        builder = new Mac_DstCriterion.Builder();
        if(builder.readMask(bb)){
            builders.put(Criterion.Type.MAC_DST, builder);
        }\n\n'''
    
data02 = '''        if(builders.containsKey(Criterion.Type.MAC_DST)){
            trafficSelector.add(builders.get(Criterion.Type.MAC_DST)
                                        .readData(bb)
                                        .build());
        }else{
            bb.skipBytes(Mac_DstCriterion.LEN);
        }\n\n'''


start_flag = 0
mid_flag = 0
end_flag = 0


def create_DefaultTrafficSelector2_1(Li_result, totle_bits):

    global start_flag 
    if(start_flag == 0):
        wtireto_start_r = wtireto_start.replace('200', str((totle_bits - 536 * 8) // 8))
        DefaultTrafficSelector2_1_path.write(wtireto_start_r)
        start_flag = 1
    for key, val in Li_result.items():
        padsize = 0
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                if (key != 'vlan1') and (key != 'vlan2'):
                    padsize += int(val_['size'])
                writeto_field_r = writeto_field_1.replace('xx', key_.upper())
                writeto_field_r = writeto_field_r.replace('yy', key_.title())
                DefaultTrafficSelector2_1_path.write(writeto_field_r) 
                data01_r = data01.replace('Mac_Dst', key_.title()) 
                data01_r = data01_r.replace('MAC_DST', key_.upper())
                DefaultTrafficSelector2_01_path.write(data01_r) 
                data02_r = data02.replace('Mac_Dst', key_.title()) 
                data02_r = data02_r.replace('MAC_DST', key_.upper())
                DefaultTrafficSelector2_02_path.write(data02_r)
                
        if padsize % 64 :
            inx = 64 - padsize % 64
            DefaultTrafficSelector2_1_path.write('        bb.writeZero(' + str(inx // 8) + ');\n\n')
            DefaultTrafficSelector2_01_path.write('        bb.skipBytes(' + str(inx // 8) + ');\n\n')
            DefaultTrafficSelector2_02_path.write('        bb.skipBytes(' + str(inx // 8) + ');\n\n') 


        


DefaultTrafficSelector2_2_path = './javafile/DefaultTrafficSelector2_2.java'
DefaultTrafficSelector2_2_path = open(DefaultTrafficSelector2_2_path, 'w', encoding='utf-8')
        
def create_DefaultTrafficSelector2_2(Li_result):
    global mid_flag 
    if(mid_flag == 0):
        DefaultTrafficSelector2_2_path.write(writeto_mid)
        mid_flag = 1
    for key, val in Li_result.items():
        padsize = 0
        bits = 0
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                bits += val_['size']
                if (key != 'vlan1') and (key != 'vlan2'):
                    padsize += int(val_['size'])
                writeto_field_r = writeto_field_2.replace('xx', key_.upper())
                writeto_field_r = writeto_field_r.replace('yy', key_.title())
                DefaultTrafficSelector2_2_path.write(writeto_field_r) 
        if padsize % 64 :
            inx = 64 - padsize % 64
            DefaultTrafficSelector2_2_path.write('        bb.writeZero(' + str(inx // 8) + ');\n\n') 
        
