import java_create_CriterionCodec

outputpath1 = './javafile/criteria/'

general_size = [8, 16, 32]
size_trans = {8: 'byte', 16: 'short', 32: 'int'}

public_criteria_class = '''package org.onosproject.net.flow.criteria;

import org.onlab.packet.short;

import io.netty.buffer.ByteBuf;

import java.util.Objects;

import java.util.Arrays;

import org.onosproject.net.flow.criteria.parser.*;

public final class XxCriterion implements Criterion {


    private final short Xx;
  	private final short mask;

    public static final int LEN = 2;

    public short value() {
        return Xx;
    }
  
    public short mask(){
        return mask;
    }

    public XxCriterion(short Xx) {
        this(Xx, 0xFFFF);
    }

    /**
     * Constructor.
     *
     * @param Xx the Ethernet frame type to match
     */
    public XxCriterion(short Xx, short mask) {
        this.Xx = Xx;
      	this.mask = mask;
    }

  	@Override
    public void write(ByteBuf bb){
        bb.writebyte((byte)Xx);
    }

    @Override
    public void writeMask(ByteBuf bb){
        bb.writebyte((byte)mask);
    }

    public static void writeZero(ByteBuf bb){
        bb.writeZero(2);
    }

    @Override
    public Type type() {
        return Type.ETH_TYPE;
    }

    /**
     * Gets the Ethernet frame type to match.
     *
     * @return the Ethernet frame type to match (16 bits unsigned integer)
     */
    public short Xx() {
        return Xx;
    }

    @Override
    public String toString() {
        return type().toString() + SEPARATOR + CriterionParser.ComplexParser(Xx.toBytes(), mask.toBytes(), type());
    }

    @Override
    public int hashCode() {
        return Objects.hash(type().ordinal(), Xx);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj instanceof XxCriterion) {
            XxCriterion that = (XxCriterion) obj;
            return Xx == that.Xx && mask == that.mask;
        }
        return false;
    }

    public static class Builder implements Criterion.Builder {
        private short Xx;
        private short mask;
        private boolean valid_mask;

        @Override
        public boolean readMask(ByteBuf bb){
            mask = bb.readbyte() & 0xFFFFL;
            if(mask != 0){
                valid_mask = true;
            }

            return valid_mask;
        }

        @Override
        public Builder setValid(boolean valid){
            valid_mask = valid;
            if(valid){ 
                this.mask = 0xFFFFL;
            }
            return this;
        }

        @Override
        public Builder readData(ByteBuf bb){
            Xx = bb.readbyte() & 0xFFFFL;
            return this;
        }

        @Override
        public XxCriterion build(){
            if(!valid_mask){
                throw new IllegalArgumentException("XxCriterion Mask should not be zero");
            }
            return new XxCriterion(Xx, mask);
        }
    }
}
'''


def create_public_final_class_implements_Criterion(Li_result):
    for key, val in Li_result.items():
        for key_, val_ in val.items():
            if (key_ != 'Type' and key_ != 'Hash' and 'crc' not in key_):
                if key_ != "srv6_segmentlist":              
                    outputpath = outputpath1 + key_.title() + "Criterion.java"
                    fo = open(outputpath, 'w', encoding='utf-8')
                    
                    public_criteria_class_r = public_criteria_class.replace('2', str(int(val_['size'])//8))
                    public_criteria_class_r = public_criteria_class_r.replace('ETH_TYPE', key_.upper())

                    if(val_['size'] not in general_size):
                        public_criteria_class_r = public_criteria_class_r.replace('this.mask = 0xFFFFL;', '''byte[] ALL = new byte[LEN];
                Arrays.fill(ALL, (byte) 0xFF);
                this.mask = short.valueOf(ALL);''')
                        public_criteria_class_r = public_criteria_class_r.replace('this(Xx, 0xFFFF);', 
                        '''byte[] ones = new byte[short.LEN];
        Arrays.fill(ones, (byte) 0xFF);
        short mask_full_one = short.valueOf(ones);
        this.Xx = Xx;
        this.mask = mask_full_one;''')
                        public_criteria_class_r = public_criteria_class_r.replace('bb.writebyte((byte)Xx);', 
                        '''byte[] data = Xx.toBytes();
        for (int i = 0; i < data.length; i++) {
            bb.writeByte(data[i]);
        }''')
                        public_criteria_class_r = public_criteria_class_r.replace('bb.writebyte((byte)mask);', 
                        '''byte[] data = mask.toBytes();
        for (int i = 0; i < data.length; i++) {
            bb.writeByte(data[i]);
        }''')
                        public_criteria_class_r = public_criteria_class_r.replace('Xx == that.Xx && mask == that.mask', key_ + 
                        '.equals(' + 'that.' + key_ + ') && mask.equals(that.mask)')
                        public_criteria_class_r = public_criteria_class_r.replace('Integer.toHexString(Xx & 0xFF)', 'Xx')
                        public_criteria_class_r = public_criteria_class_r.replace('''mask = bb.readbyte() & 0xFFFFL;
            if(mask != 0){
                valid_mask = true;
            }''', '''byte[] mask = new byte[LEN];
            valid_mask = false;
            for(int i = 0; i < LEN; i++){
                byte b = bb.readByte();
                if(b != (byte)0x0){
                    valid_mask = true;
                }
                mask[i] = b;
            }
            if(valid_mask)
                this.mask = short.valueOf(mask);''')
                        public_criteria_class_r = public_criteria_class_r.replace('Xx = bb.readbyte() & 0xFFFFL;', '''byte[] data = new byte[LEN];
            bb.readBytes(data);
            this.Xx = short.valueOf(data);''')
                        public_criteria_class_r = public_criteria_class_r.replace('short', key_.title())
                    else:
                        if val_['size'] == 64: 
                            print(key_, val_['size'])
                        public_criteria_class_r = public_criteria_class_r.replace('import org.onlab.packet.short;', ' ')
                        public_criteria_class_r = public_criteria_class_r.replace('FFFF', 'FF'*(int(val_['size'])//8)) 
                        public_criteria_class_r = public_criteria_class_r.replace('Short', 'long'.title())
                        public_criteria_class_r = public_criteria_class_r.replace('short', 'long') 
                        public_criteria_class_r = public_criteria_class_r.replace('readbyte', 'read' + size_trans[int(val_['size'])].title())
                        public_criteria_class_r = public_criteria_class_r.replace('writebyte', 'write' + size_trans[int(val_['size'])].title())
                        public_criteria_class_r = public_criteria_class_r.replace('byte', size_trans[int(val_['size'])])
                        public_criteria_class_r = public_criteria_class_r.replace('Complex', 'Basic')
                        public_criteria_class_r = public_criteria_class_r.replace('Xx.toBytes()', 'Xx')
                        public_criteria_class_r = public_criteria_class_r.replace('mask.toBytes()', 'mask')
                        # if int(val_['size']) == 64:
                        #     public_criteria_class_r = public_criteria_class_r.replace('Integer', 'Long')
                        # if int(val_['size']) == 64 or int(val_['size']) == 32:   
                        #     public_criteria_class_r = public_criteria_class_r.replace(' & 0xFF', '')
                        # elif int(val_['size']) == 16:
                        #     public_criteria_class_r = public_criteria_class_r.replace(' & 0xFF', ' & 0xFFFF')
                    public_criteria_class_r = public_criteria_class_r.replace('XxCriterion', key_.title() + 'Criterion')
                    public_criteria_class_r = public_criteria_class_r.replace('Xx', key_)
                    fo.write(public_criteria_class_r)
                else:
                    srv6_segl = int(val_['size']) % 128
                    while srv6_segl > 0:      
                        outputpath = outputpath1 + key_.title() + str(srv6_segl) + "Criterion.java"
                        fo = open(outputpath, 'w', encoding='utf-8')
                        srv6_size = srv6_segl * 128
                        public_criteria_class_r = public_criteria_class.replace('this.mask = 0xFFFFL;', '''byte[] ALL = new byte[LEN];
                Arrays.fill(ALL, (byte) 0xFF);
                this.mask = short.valueOf(ALL);''')
                        public_criteria_class_r = public_criteria_class_r.replace('this(Xx, 0xFFFF);', 
                        '''byte[] ones = new byte[short.LEN];
        Arrays.fill(ones, (byte) 0xFF);
        short mask_full_one = short.valueOf(ones);
        this.Xx = Xx;
        this.mask = mask_full_one;''')
                        public_criteria_class_r = public_criteria_class_r.replace('2', str(128//8))
                        public_criteria_class_r = public_criteria_class_r.replace('ETH_TYPE', (key_ + str(srv6_segl)).upper())

                        public_criteria_class_r = public_criteria_class_r.replace('bb.writebyte((byte)Xx);', 
                        '''byte[] data = Xx.toBytes();
        for (int i = 0; i < data.length; i++) {
            bb.writeByte(data[i]);
        }''')
                        public_criteria_class_r = public_criteria_class_r.replace('bb.writebyte((byte)mask);', 
                        '''byte[] data = mask.toBytes();
        for (int i = 0; i < data.length; i++) {
            bb.writeByte(data[i]);
        }''')
                        public_criteria_class_r = public_criteria_class_r.replace('Integer.toHexString(Xx & 0xFF)', 'Xx')
                        public_criteria_class_r = public_criteria_class_r.replace('Xx == that.Xx && mask == that.mask', 
                                                                                key_ + str(srv6_segl) + '.equals(' + 'that.' 
                                                                                + key_ + str(srv6_segl) + ') && mask.equals(that.mask)')
                        public_criteria_class_r = public_criteria_class_r.replace('''mask = bb.readbyte() & 0xFFFFL;
            if(mask != 0){
                valid_mask = true;
            }''', '''byte[] mask = new byte[LEN];
            valid_mask = false;
            for(int i = 0; i < LEN; i++){
                byte b = bb.readByte();
                if(b != (byte)0x0){
                    valid_mask = true;
                }
                mask[i] = b;
            }
            if(valid_mask)
                this.mask = short.valueOf(mask);''')
                        public_criteria_class_r = public_criteria_class_r.replace('Xx = bb.readbyte() & 0xFFFFL;', '''byte[] data = new byte[LEN];
            bb.readBytes(data);
            this.Xx = short.valueOf(data);''')                  
                        public_criteria_class_r = public_criteria_class_r.replace('short', key_.title() + str(srv6_segl))
                        public_criteria_class_r = public_criteria_class_r.replace('XxCriterion', key_.title() + str(srv6_segl) 
                                                                                    + 'Criterion')
                        public_criteria_class_r = public_criteria_class_r.replace('Xx', key_ + str(srv6_segl))
                        fo.write(public_criteria_class_r)
                        srv6_segl -= 1