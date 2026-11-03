outputpath = './javafile/instructions/Instructions.java'

start = '''    public static AddProtocolInstruction createAddProtocol(int flag, Protocol protocol) {
        checkNotNull(protocol, "protocol cannot be null");
        return new AddProtocolInstruction(flag, protocol);
    }

    public static AddProtocolInstruction createAddProtocol(String protocol, TrafficSelector selector) {
        int flag = Protocol.ProtocolFormatByString(protocol);
        switch(flag){\n'''

data1_1 = '''            case Protocol.MAC:
                checkNotNull(selector, "selector cannot be null");\n'''
data1_2 = '''                Mac_DstCriterion mac_dst = (Mac_DstCriterion)selector.getCriterion(Criterion.Type.MAC_DST);
                checkNotNull(mac_dst, "Action ADD_PROTOCOL need MAC_DST field");\n'''

data2_1 = '                return new AddProtocolInstruction(flag, new Mac_Protocol('
data2_2 = 'mac_dst, '
data2_3 = '                return new ModFieldInstruction(flag, new Mac_Protocol('

mid1 = '''            default:
                throw new UnsupportedOperationException("Action ADD_PROTOCOL add a unsupported protocol");
        }
    }

    public static class AddProtocolInstruction implements Instruction {
        private final int flag;
        private final Protocol protocol;

        AddProtocolInstruction(int flag, Protocol protocol) {
            this.flag = flag;
            this.protocol = protocol;
        }

        public static AddProtocolInstruction readFrom(ByteBuf bb){
            int start = bb.readerIndex();

            byte type = bb.readByte();
            byte raw = bb.readByte();
            short len = bb.readShort();
            int protocol_type = Integer.reverseBytes(bb.readInt());
            Protocol result = null;
            switch(protocol_type){\n'''

data3 = '''                case Protocol.MAC:
                    result = Mac_Protocol.read(bb);
                    break;\n'''

mid2 = '''                default:
                    throw new UnsupportedOperationException("Action ADD_PROTOCOL add a unsupported protocol");
            }

            int pad = 8 - ((bb.readerIndex() - start) % 8);
            if(pad % 8 != 0)
                bb.skipBytes(pad); 

            return createAddProtocol(protocol_type, result);
        }

        @Override
        public void write(ByteBuf bb){
            int start = bb.writerIndex();
            // type 
            bb.writeByte((byte)59);
            // raw
            bb.writeByte(0xff);
            // len
            int lengthIndex = bb.writerIndex();
            bb.writeShort((short)0);
            // flag
            bb.writeInt(Integer.reverseBytes(flag));
            // protocol
            protocol.write(bb);
            //pad
            int pad = 8 - ((bb.writerIndex() - start) % 8);
            if(pad % 8 != 0)
                bb.writeZero(pad);

            int length = bb.writerIndex() - start;
            bb.setShort(lengthIndex, Short.reverseBytes((short)length));
        }

        @Override
        public Type type() {
            return Type.ADD_PROTOCOL;
        }

        @Override
        public String toString() {
            return "AddProtocol[" + protocol.toString() + "]";
        }

        @Override
        public int hashCode() {
            return Objects.hash(type().ordinal(), flag);
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj instanceof AddProtocolInstruction) {
                AddProtocolInstruction that = (AddProtocolInstruction) obj;
                return Objects.equals(flag, that.flag) && Objects.equals(protocol, that.protocol);

            }
            return false;
        }
    }

    public static ModFieldInstruction createModField(int flag, Protocol protocol) {
        checkNotNull(protocol, "protocol cannot be null");

        return new ModFieldInstruction(flag, protocol);

    }

    public static ModFieldInstruction createModField(String protocol, TrafficSelector selector) {
        int flag = Protocol.ProtocolFormatByString(protocol);
        switch(flag){\n'''

mid3 = '''            default:
                throw new UnsupportedOperationException("Action MOD_FIELD add a unsupported protocol");
        }

    }


    public static class ModFieldInstruction implements Instruction {
        private final int flag;
        private final Protocol protocol;

        ModFieldInstruction(int flag, Protocol protocol) {
            this.flag = flag;
            this.protocol = protocol;
        }
        
        public static ModFieldInstruction readFrom(ByteBuf bb){
            int start = bb.readerIndex();
        
            byte type = bb.readByte();
            byte raw = bb.readByte();
            short len = bb.readShort();
            int protocol_type = Integer.reverseBytes(bb.readInt());
        
            int protocolLength = bb.readerIndex();
            Protocol result = null;
            switch(protocol_type){\n'''

end = '''               default:
                    throw new UnsupportedOperationException("Action MOD_FIELD add a unsupported protocol");
            }
        
            int pad = 8 - ((bb.readerIndex() - start) % 8);
            if(pad % 8 != 0)
                bb.skipBytes(pad); 
        
            return createModField(protocol_type, result);
        }
        
        @Override
        public void write(ByteBuf bb){
            int start = bb.writerIndex();
            // type 
            bb.writeByte((byte)61);
            // raw
            bb.writeByte(0xff);
            // len
            int lengthIndex = bb.writerIndex();
            bb.writeShort((short)0);
            // flag
            bb.writeInt(Integer.reverseBytes(flag));
            // protocol
            protocol.write(bb);
            // mask
            protocol.writeMask(bb);
            //pad
            int pad = 8 - ((bb.writerIndex() - start) % 8);
            if(pad % 8 != 0)
                bb.writeZero(pad);
        
            int length = bb.writerIndex() - start;
            bb.setShort(lengthIndex, Short.reverseBytes((short)length));
        }
        
        @Override
        public Type type() {
            return Type.MOD_FIELD;
        }
        
        @Override
        public String toString() {
            return "ModField[" + protocol.toString() + "]";
        }
        
        @Override
        public int hashCode() {
            return Objects.hash(type().ordinal(), flag);
        }
        
        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (obj instanceof ModFieldInstruction) {
                ModFieldInstruction that = (ModFieldInstruction) obj;
                return Objects.equals(flag, that.flag) && Objects.equals(protocol, that.protocol);
        
            }
            return false;
        }

    }
}\n'''


def create_Instructions_start():
    with open(outputpath, 'w') as outfile:
        with open('./javafile/Instructions_ori', 'r') as infile:
            outfile.write(infile.read())
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(start)


def create_Instructions1(Li_result, times):
    for key, val in Li_result.items():
        try:
            del val['Type']
        except KeyError as e:
            a = 1
        try:
            del val['Hash']
        except KeyError as e:
            a = 1
        if key != 'srv6':
            data1_1r = data1_1.replace('MAC', key.upper())
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data1_1r)   
            for key_, val_ in val.items():
                data1_2r = data1_2.replace('mac_dst', key.upper() + '_' + key_)
                data1_2r = data1_2r.replace('Mac_Dst', key_.title())
                data1_2r = data1_2r.replace('MAC_DST', key_.upper())
                if times == 3:
                    data1_2r = data1_2r.replace('ADD_PROTOCOL', 'MOD_FIELD')
                with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                    file_obj.write(data1_2r)
            if times == 1:
                data2_1r = data2_1.replace('Mac', key.title())
                with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                    file_obj.write(data2_1r)
            elif times == 3:
                data2_3r = data2_3.replace('Mac', key.title())
                with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                    file_obj.write(data2_3r)
            keys = list(val.keys())
            last_key = keys[-1]
            for key_, val_ in val.items():   
                if key_ != last_key:
                    data2_2r = data2_2.replace('mac_dst', key.upper() + '_' + key_)
                    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                        file_obj.write(data2_2r) 
                else:
                    data2_2r = data2_2.replace('mac_dst', key.upper() + '_' + key_)
                    data2_2r = data2_2r.replace(', ', '));\n')
                    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                        file_obj.write(data2_2r)
        else:
            for key_, val_ in val.items():
                if key_ == "srv6_segmentlist": 
                    srv6_segl = int(val_['size']) % 128
                    for i in range(0, srv6_segl):
                        data1_1r = data1_1.replace('MAC', key.upper() + '_' + str(i + 1))
                        with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data1_1r)
                        for key_, val_ in val.items():
                            if key_ != "srv6_segmentlist":
                                data1_2r = data1_2.replace('mac_dst', key.upper() + '_' + str(i + 1) + '_' + key_)
                                data1_2r = data1_2r.replace('Mac_Dst', key_.title())
                                data1_2r = data1_2r.replace('MAC_DST', key_.upper())
                                if times == 3:
                                        data1_2r = data1_2r.replace('ADD_PROTOCOL', 'MOD_FIELD')
                                with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                    file_obj.write(data1_2r)
                            else:
                                for j in range(1, i + 2):
                                    data1_2r = data1_2.replace('mac_dst', key.upper() + '_' + str(i + 1) + '_' + key_ + str(j))
                                    data1_2r = data1_2r.replace('Mac_Dst', key_.title() + str(j))
                                    data1_2r = data1_2r.replace('MAC_DST', key_.upper() + str(j))
                                    if times == 3:
                                        data1_2r = data1_2r.replace('ADD_PROTOCOL', 'MOD_FIELD')
                                    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                        file_obj.write(data1_2r)
                        if times == 1:
                            data2_1r = data2_1.replace('Mac', key.title() + '_' + str(i + 1))
                            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                file_obj.write(data2_1r)
                        elif times == 3:
                            data2_3r = data2_3.replace('Mac', key.title() + '_' + str(i + 1))
                            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                file_obj.write(data2_3r)
                        keys = list(val.keys())
                        last_key = keys[-1]
                        for key_, val_ in val.items():  
                            if key_ != "srv6_segmentlist": 
                                data2_2r = data2_2.replace('mac_dst', key.upper() + '_' + str(i + 1) + '_' + key_)
                                with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                    file_obj.write(data2_2r) 
                            else:
                                for j in range(1, i + 1):
                                    data2_2r = data2_2.replace('mac_dst', key.upper() + '_' + str(i + 1) + '_' + key_ + str(j))
                                    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                        file_obj.write(data2_2r) 
                                data2_2r = data2_2.replace('mac_dst', key.upper() + '_' + str(i + 1) + '_' + key_ + str(i + 1))
                                data2_2r = data2_2r.replace(', ', '));\n')
                                with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                                    file_obj.write(data2_2r)
                            


def create_Instructions2(Li_result, times):
    for key, val in Li_result.items():
        try:
            del val['Type']
        except KeyError as e:
            a = 1
        try:
            del val['Hash']
        except KeyError as e:
            a = 1
        if key != 'srv6':
            data3r = data3.replace('MAC', key.upper())
            data3r = data3r.replace('Mac', key.title())
            if times == 3:
                data3r = data3r.replace('read', 'readWithMask')
            with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                file_obj.write(data3r)
        else:
            for key_, val_ in val.items():
                if key_ == "srv6_segmentlist": 
                    srv6_segl = int(val_['size']) % 128
                    for i in range(0, srv6_segl):
                        data3r = data3.replace('MAC', key.upper() + '_' + str(i + 1))
                        data3r = data3r.replace('Mac', key.title() + '_' + str(i + 1))
                        if times == 3:
                            data3r = data3r.replace('read', 'readWithMask')
                        with open(outputpath, mode='a', encoding='utf-8') as file_obj:
                            file_obj.write(data3r)


def create_Instructions_mid1():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid1)


def create_Instructions_mid2():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid2)
        

def create_Instructions_mid3():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid3)


def create_Instructions_end():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(end)