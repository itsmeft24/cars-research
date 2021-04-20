import sys, struct

#STRUCT_DEFINITIONS
HEADER = ">4siihhiiii"
LABEL_DEF = ">IIHHI"
CHILD_DEF = ">IIHHI"

def readString(myfile):
    str_list = []
    while True:
        str = myfile.read(1)
        if bytes(str) == b'\x00':
            return "".join(str_list)
        str_list.append(str.decode('ascii'))

param_block = open(sys.argv[1], "rb")
output = open(sys.argv[2], "w")
param_header = struct.unpack(HEADER, param_block.read(32))

MAGIC = param_header[0]


labeldef_offset = param_header[1]
childdef_offset = param_header[2]
numlabels = param_header[3]
numoptions = param_header[4]
labelstrings_offset = param_header[5]
labelstrings_size = param_header[6]
optionstrings_offset = param_header[7]
optionstrings_size = param_header[8]

param_block.seek(labeldef_offset)

if MAGIC == b"BPBh":
    print("Race-O-Rama File Detected!")
    RACE_O_RAMA = True
elif:
    print("Mater-National Detected!")
    RACE_O_RAMA = False
else:
    print("Invalid BPB File!")
    exit(-1)

for x in range(numlabels):
    param_block.seek(x*16+0x20)#Seek to the offset of the definition and unpack it into a tuple.

    entry_data = struct.unpack(LABEL_DEF, param_block.read(0x10))
    param_block.seek(x*16+0x20)

    UNKNOWN = entry_data[0]
    string_offset = entry_data[1]
    flag = entry_data[2]
    number_of_children = entry_data[3]# Could also hold type definition for Options/Children
    first_option_index = entry_data[4]

    param_block.seek(string_offset+labelstrings_offset)
    name = readString(param_block)
    param_block.seek(x*16+0x20)
    if flag == 65535:# Is a "Normal" Label Definition?
        output.write("["+name+"]\n")
    else:
        output.write("["+name+str(flag)+"]\n")
    offset_seek = (first_option_index)*16+childdef_offset
    for y in range(number_of_children):
        
        param_block.seek(offset_seek+y*16)
        entry_data = struct.unpack(LABEL_DEF, param_block.read(0x10))
        UNKNOWN = entry_data[0]
        string_offset = entry_data[1]

        param_block.seek(offset_seek+4+y*16)
        extra_string_offset = param_block.read(1)
        if extra_string_offset != 0:
            param_block.seek(offset_seek+5+y*16)
            string_offset = struct.unpack('>I', b"\x00"+param_block.read(3))[0]
        
        # Race-O-Rama Flags
        # 0x40 value is right after
        # 0x80 is int
        # 0xc0 is float
        # 0x00 value offset

        flag = entry_data[2]
        value_offset = entry_data[4]
        param_block.seek(string_offset+labelstrings_offset)
        name = readString(param_block)
        param_block.seek(offset_seek+y*16)
        
        if RACE_O_RAMA:
            if flag == 65535:
                OPTION_INDEX = ""
            else:
                OPTION_INDEX = str(flag)
            
            if extra_string_offset == b"\x40":
                param_block.seek(0xA, 1)
                value = readString(param_block)
                output.write(name+OPTION_INDEX+"="+value+"\n")
            elif extra_string_offset == b"\x80":
                param_block.seek(0xC, 1)
                value = str(struct.unpack('>I', param_block.read(4))[0])
                output.write(name+OPTION_INDEX+"="+value+"\n")
            elif extra_string_offset == b"\xC0":
                param_block.seek(0xC, 1)
                value = str(struct.unpack('>f', param_block.read(4))[0])
                output.write(name+OPTION_INDEX+"="+value+"\n")
            elif extra_string_offset == b"\x00":
                param_block.seek(value_offset+optionstrings_offset)
                value = readString(param_block)
                output.write(name+OPTION_INDEX+"="+value+"\n")
            else:
                print("UNKNOWN_FLAG of type RACE_O_RAMA: "+str(extra_string_offset))
        else:
        
            if flag == 65535:
                print("Flag 0xFF found! Please report this file to @data.arc#5576 on Discord!")
                # param_block.seek(0xC, 1)
                # value = str(struct.unpack('>I', param_block.read(4))[0])
                # param_block.seek(int(value)-1+optionstrings_offset)
                # char = param_block.read(1)

                # if char == b"\x00":
                    # param_block.seek(int(value)+optionstrings_offset)
                    # value = readString(param_block)
                    # output.write(name+"="+value+"\n")
                # else:
                    # output.write(name+"="+value+"\n")
            elif flag == 1:# Flag is 0x01?
                param_block.seek(value_offset+optionstrings_offset)
                value = readString(param_block)
                output.write(name+"="+value+"\n")
            elif flag == 2:# Flag is 0x02?
                param_block.seek(0xA, 1)
                value = readString(param_block)
                output.write(name+"="+value+"\n")
            elif flag == 8:# Flag is 0x08?
                param_block.seek(0xC, 1)
                value = str(struct.unpack('>f', param_block.read(4))[0])
                output.write(name+"="+value+"\n")
            elif flag == 4:# Flag is 0x04?
                param_block.seek(0xC, 1)
                value = str(struct.unpack('>I', param_block.read(4))[0])
                output.write(name+"="+value+"\n")
            else:
                output.write(name+" UNKNOWN_FLAG:"+hex(flag)+"\n")
