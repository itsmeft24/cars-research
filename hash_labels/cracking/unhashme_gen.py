import struct, sys

IS_PS3 = False

def djb_hash(str):
    if IS_PS3:
        str = str.replace("\\", "/").lower()
    else:
        str = str.lower()
    hash = 5381
    for x in range(len(str)):
        hash = (((hash << 5) + hash) + ord(str[x])) & 0xFFFFFFFF
    return hash

def bytesearch(needle, haystack):
    return haystack.find(needle)

def getext(file):
    magic = file[:3]
    if magic == b"BPB":
        return ".bpb"
    elif magic == b"RIF":
        return ".xma"
    elif magic == b"MSF":
        return ".msf"
    elif magic == b"DDS":
        return ".dds"
    elif magic == b"333":
        return ".cel"
    elif magic == b"\x89PN":
        return ".png"
    elif magic == b"FOR":
        return ".aiff"
    elif magic == b"BIN":
        return ".mcf.bin"
    elif magic == b"BXM":
        return ".bxml"
    elif magic == b"ZLI":
        return ".bpb.zlib"
    elif magic == b"\xFF\xFF\xFF":
        return ".got"
    elif magic == b"pat":
        return ".tcs_script_for_wii_shit"
    elif magic == b"th_":
        return ".more_wii_shit"
    elif magic == b"War":
        return ".debug_shit"
    elif magic == b"WIB":
        return ".wii_banner"
    elif magic == b"xng":
        return ".xng"
    elif magic == b"p3g":
        return ".p3g"
    elif magic == b"#in":
        return ".cpp_thats_what_bms_said"
    elif magic == b"\x03\x00\x00":
        return ".res"
    elif magic == b"\x00ff":
        return ".col"
    elif magic == b"\xD2\x04\x00":
        return ".col"
    elif magic == b"\x02\x00\x00":
        return ".esb"
    elif magic == b"\x01\x00\x00":
        return ".ess"
    elif magic[:2] == b"\x78\x9C":
        return ".str"
    elif b"\xFF\xFE" in file[:6]:
        return ".LOC_BIN"
    elif bytesearch(b"\x00\x00\x1B\x5C", file) !=-1:
        return ".fpo"
    elif bytesearch(b"\x00\x00\x1B\x5B", file) !=-1:
        return ".vpo"    
    elif magic == b"\x10\x2A\x11":
        if bytesearch(b"VS_", file) !=-1:
            return ".vsh"
        elif bytesearch(b"PS_", file) !=-1:
            return ".psh"
        else:
            if bytesearch(b"vs_", file) !=-1:
                return ".vsh"
            elif bytesearch(b"ps_", file) !=-1:
                return ".psh"
            else:
                return ".a_shader"
    else:
        if bytesearch(b"LuaQ", file) != -1:
            return ".lua.bin"
        elif bytesearch(b"local", file) != -1:
            return ".lua"
        elif bytesearch(b"TotalShaderPasses", file) != -1:
            return ".xnm"
        elif bytesearch(b"ParticleGroupName", file) != -1:
            return ".emt"
        elif bytesearch(b"RaceTrack", file) != -1:
            return ".trk"
        elif bytesearch(b"NumberOfMotions", file) != -1:
            return ".mcf"
        elif bytesearch(b"StyleSheet", file) != -1:
            return ".pag"
        elif bytesearch(b"Activity", file) != -1:
            return ".aty"
        elif bytesearch(b"CarsAnimatedToys", file) != -1:
            return ".toy"
        elif bytesearch(b"\x00\x00\x04\xD2col", file) != -1:
            return ".gol"
        elif bytesearch(b"Quantization Info", file) != -1:
            return ".slt"
        elif bytesearch(b"Materials", file) != -1:
            return ".mtl"
        elif bytesearch(b"DDS |", file) != -1:
            return ".atd"
        else:
            return ".ini"

hash_labels = open("../xbr_hashes.txt", "r").read().split("\n")

label_dict = {}

for x in range(len(hash_labels)):
    hash = djb_hash(hash_labels[x])
    label_dict[hash] = hash_labels[x]

xbr = open("cars3.xbr", "rb")

xbr_header = struct.unpack(">IIIIII", xbr.read(24))

version = xbr_header[0]
number_files = xbr_header[1]
DUMMY = xbr_header[2]
DUMMY_0 = xbr_header[3]
info_size = xbr_header[4]
zero = xbr_header[5]

for x in range(number_files):
    xbr.seek(24+12*x)
    table_entry = struct.unpack(">III", xbr.read(12))
    HASH = table_entry[0]
    SIZE = table_entry[1]
    OFFSET = table_entry[2]
    try:
        label_dict[HASH]
    except KeyError:
        xbr.seek(OFFSET * 0x800)
        data = xbr.read(SIZE)
        file = open("./unhashme/"+hex(HASH)+getext(data), "wb")
        file.write(data)
        file.close()
        print(hex(HASH))