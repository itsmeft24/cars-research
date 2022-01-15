import sys, struct, zlib, io, numpy as np

if len(sys.argv) < 3:
    print("Cars .BPB (BinaryParameterBlock) to .PB (ParameterBlock) Converter\n\nUsage: python bpbconv.py <INPUT_BPB> <OUTPUT_FILE>")
    exit()

# STRUCT DEFINITIONS
HEADER = ">4siihhiiii"
PARENT_DEFINITION = ">IIHHI"

def readString(file):
    string = b""
    while True:
        str = file.read(1)
        if str == b'\x00':
            return string.decode(encoding="ascii", errors='ignore')
        string = string + str

def djb_hash(str):
    str = str.lower()
    hash = 5381
    for x in range(len(str)):
        hash = (((hash << 5) + hash) + ord(str[x]))& 0xFFFFFFFF
    return hash 

param_block = open(sys.argv[1], "rb")

if param_block.read(0x6) == b"ZLIBME":
    param_block.seek(0x10)
    param_block = io.BytesIO(zlib.decompress(param_block.read()))
param_block.seek(0)
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
elif MAGIC == b"BPB\x00":
    print("Mater-National Detected!")
    RACE_O_RAMA = False
else:
    print("Invalid BPB File!")
    exit(-1)

output = open(sys.argv[2], "w")

# Read the string tables and add entries into a dictionary for cleaner lookups.
param_block.seek(labelstrings_offset)

hash_labels = {}

labels = param_block.read(labelstrings_size).split(b"\x00")
param_block.seek(optionstrings_offset)
labels = labels + param_block.read(optionstrings_size).split(b"\x00")

for x in range(len(labels)):
    hash_labels[djb_hash(labels[x].decode())] = labels[x].decode()

del labels

for x in range(numlabels):
    param_block.seek(x*16+0x20) # Seek to the offset of the definition and unpack it into a tuple

    entry_data = struct.unpack(PARENT_DEFINITION, param_block.read(0x10))

    hash = entry_data[0] # This format uses DJB-2 hashes to speed up parameter lookups
    string_offset = entry_data[1]
    flag = entry_data[2]
    number_of_children = entry_data[3]
    first_option_index = entry_data[4]
    
    try:
        name = hash_labels[hash]
    except:
        name = hex(hash)
    
    if flag == 65535:
        if x == 0: # Check if its not the first loop iteration, and add a newline at the beginning of the parent
            output.write("["+name+"]\n")
        else:
            output.write("\n["+name+"]\n")
    else: # Append parent suffix (typically an index)
        if x == 0: # Check if its not the first loop iteration, and add a newline at the beginning of the parent
            output.write("["+name+str(flag)+"]\n")
        else:
            output.write("\n["+name+str(flag)+"]\n")
    
    offset_seek = (first_option_index)*16+childdef_offset
    for y in range(number_of_children):
        
        param_block.seek(offset_seek+y*16)

        hash = int.from_bytes(param_block.read(4), "big") # This format uses DJB-2 hashes to speed up parameter lookups
        ror_flag = int.from_bytes(param_block.read(1), "big") # Only used in Race-O-Rama BPBs
        string_offset = int.from_bytes(param_block.read(3), "big")
        # This might cause some problems later down the road, but unlikely. If the game can only use 24 bits for the string offset, then this should be fine.
        # Or, both the flag and string offset are actually 16 bits and im just crazy!
        
        mn_flag = int.from_bytes(param_block.read(2), "big", signed=False) # Flag in MN, child suffix in ror
        reserved = param_block.read(2) # null unless a string is stored right inside the parent definition
        value_buf = param_block.read(4) # Can be an offset into the value string table, an int, or a float. its parsed into its appropriate type in the following if block.
        
        try:
            name = hash_labels[hash]
        except:
            name = hex(hash) # Used as a fallback, useful if we ever try modifying ror's save data
        
        param_block.seek(offset_seek+y*16)
        
        if RACE_O_RAMA:
            if mn_flag == 65535:
                pass
            else:
                name+=str(mn_flag) # Append child suffix (typically an index)
            
            if ror_flag == 0x00: # Offset into value string table
                param_block.seek(int.from_bytes(value_buf, "big", signed=False)+optionstrings_offset)
                value = readString(param_block)
                if " " in value:
                    output.write(name+"="+'"'+value+'"\n')
                else:
                    output.write(name+"="+value+"\n")
            elif ror_flag == 0x40: # String stored inside the child definition
                value = (reserved + value_buf).replace(b"\x00", b"").decode(encoding="ascii", errors='ignore')
                if " " in value:
                    output.write(name+"="+'"'+value+'"\n')
                else:
                    output.write(name+"="+value+"\n")
            elif ror_flag == 0x80: # Integer
                output.write(name+"="+str(int.from_bytes(value_buf, "big", signed=True))+"\n")
            elif ror_flag == 0xC0: # Float
                value = str(np.float32(struct.unpack('>f', value_buf)[0])) # Truncate float using NumPy
                output.write(name+"="+value+"\n")
            else:
                print("Unknown Race-O-Rama type flag!: "+hex(ror_flag)+" Please open an issue on the https://www.github.com/itsmeft24/cars-research repository! (Be sure to attach the file!)")
        else:
            if mn_flag == 0x01: # Offset into value string table
                param_block.seek(int.from_bytes(value_buf, "big", signed=False)+optionstrings_offset)
                value = readString(param_block)
                if " " in value:
                    output.write(name+"="+'"'+value+'"\n')
                else:
                    output.write(name+"="+value+"\n")
            elif mn_flag == 0x02: # String stored inside the child definition
                value = (reserved + value_buf).replace(b"\x00", b"").decode(encoding="ascii", errors='ignore')
                if " " in value:
                    output.write(name+"="+'"'+value+'"\n')
                else:
                    output.write(name+"="+value+"\n")
            elif mn_flag == 0x04: # Integer
                output.write(name+"="+str(int.from_bytes(value_buf, "big", signed=True))+"\n")
            elif mn_flag == 0x08: # Float
                value = str(np.float32(struct.unpack('>f', value_buf)[0])) # Truncate float using NumPy
                output.write(name+"="+value+"\n")
            else:
                print("Unknown Mater-National type flag!: "+mn_flag+" Please open an issue on the https://www.github.com/itsmeft24/cars-research repository! (Be sure to attach the file!)")
output.close()
