import struct, sys, os, zlib

if len(sys.argv) < 3:
    print("Cars BINMCF Unpacker")
    print("Usage: python binmcf.py <BIN_MCF> <OUT_DIR>")
    exit(-1)

def readString(file):
    string = b""
    while True:
        char = file.read(1)
        if char == b'\x00':
            return string.decode('ascii')
        string = string + char

def create_file(path):
    parent = path.replace(path.split("\\")[-1], "")
    if os.path.exists(parent):
        pass
    else:
        os.makedirs(parent)
    return open(path, "wb")

os.makedirs(sys.argv[2], exist_ok=True)

binmcf = open(sys.argv[1], "rb")

header = struct.unpack(">QIIIIII", binmcf.read(0x20))

mcf_offset = header[1]
mcf_size = header[2]
table_offset = header[3]
number_of_anims = header[4]
string_table_offset = header[5]
string_table_size = header[6]

binmcf.seek(mcf_offset)

mcf_data = binmcf.read(mcf_size)
mcf_name = sys.argv[1][0:sys.argv[1].find(".mcf")+4]+".bpb"
mcf_name = os.path.join(sys.argv[2]+"\\", mcf_name)
print(sys.argv[2])
print(mcf_name)

mcf = open(mcf_name, "wb")
mcf.write(mcf_data)
mcf.close()

for x in range(number_of_anims):

    binmcf.seek(table_offset+16*x)
    
    table_entry = struct.unpack(">IIII", binmcf.read(16))
    anim_offset = table_entry[0]
    comp_size = table_entry[1]
    decomp_size = table_entry[2]
    name_offset = table_entry[3]
    
    binmcf.seek(anim_offset)
    if comp_size == 0:
        break
    anim_data = zlib.decompress(binmcf.read(comp_size))
    
    binmcf.seek(string_table_offset + name_offset)
    name = readString(binmcf)
    
    anim_file = create_file(os.path.join(sys.argv[2], name))
    anim_file.write(anim_data)
    anim_file.close()
