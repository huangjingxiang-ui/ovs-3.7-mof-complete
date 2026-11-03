outputpath = './javafile/mof/MOFFlow.java'

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

import static com.google.common.base.Preconditions.checkNotNull;
/**
* MOFFlow Packet.
*/
public class MOFFlow {
    private int parsed_length = 0;\n'''

data = ['''    private MOFL3Layer l3_layer;\n''', '''    public void setL3Layer(MOFL3Layer l3){
        this.l3_layer = l3;
    }\n''', '''    public MOFL3Layer getL3Layer(){
        checkNotNull(l3_layer, "get MOFL3Layer value is null");
        return l3_layer;
    }\n''', '''        if(next_type != -1){
            next_type = parseL3(next_type, data);
        }\n''', '''    private int parseL3(int type, byte[] data){
        MOFL3Layer l3 = MOFL3Layer.parse(type, data, this.parsed_length);
        setL3Layer(l3);

        this.parsed_length += l3.Length();
        return l3.getNextType();
    }\n\n''']

mid1 = '''\n    /**
    * By default, set MOFFlow to untagged.
    */
    public MOFFlow(int offset) {
        parsed_length = offset;
    }\n\n'''

mid2 = '''\n    public void parser(int type, byte[] data){
        int next_type = parseL3(type, data);\n'''

mid3 = '    }\n\n'

end = '}'

datar = []

def create_MOFFlow(Layeri, lay, num):
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(data[num].replace('3', lay))


def create_MOFFlow_start():
    with open(outputpath, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(start)


def create_MOFFlow_mid1():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid1)


def create_MOFFlow_mid2():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid2)


def create_MOFFlow_mid3():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(mid3)


def create_MOFFlow_end():
    with open(outputpath, mode='a', encoding='utf-8') as file_obj:
        file_obj.write(end)