

sHash = '    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, fieldname[cnt]));\n'

sHeader = '\n\nuint32_t miniflow_hash(const struct miniflow *flow)\n\
{\n\
	uint32_t hash = 0;\n'

sTail = '    return hash_finish(hash, 42);\n\
}\n\n'
def sModHash(sWholeStr,name,size):
    dSizeToType = {8 : 'U8', 16 :'U16', 32 : 'U32',64 : 'U64'}   
    # print("name:",name)
    # print("size:",size)
    cnt = size / 8
    for i in range(int(cnt)):
        sWholeStr = sWholeStr + '    hash = hash_add(hash, MINIFLOW_GET_U8_MOF(flow, '+name+'['+str(i)+']));\n'
    #sWholeStr = sHash.replace('TYPE',dSizeToType[size])
    #sWholeStr = sWholeStr.replace('fieldname', name)
    
    return sWholeStr

def vCreateHash(dHead,name):
    file_path = './cfile/hash.c'
    if(name != None):
        pass
        #print(1)
    else:
        for fname,val in dHead.items():
            dHeadVal = val
        if('Hash' not in dHeadVal.keys()):
            return 
        else:
            sWholeStr = '\n'
            hashname = dHeadVal['Hash']
            for hn in hashname:
                sWholeStr =  sModHash(sWholeStr,hn,dHeadVal[hn]['size'])
        #         print (sWholeStr)
        #         print("-------------------")

        # print("*****************")
        with open(file_path, mode='a', encoding='utf-8') as file_obj:
            file_obj.write(sWholeStr)

def vHashInit():
    global sHeader
    #先打开parser.c
    file_path = './cfile/hash.c'
    with open(file_path, mode='w', encoding='utf-8') as file_obj:
        file_obj.write(sHeader)

def vHashFinish():
    global sTail
    #先打开parser.c
    file_path = './cfile/hash.c'
    with open(file_path, mode='a', encoding='utf-8') as file_obj:
            file_obj.write(sTail)
