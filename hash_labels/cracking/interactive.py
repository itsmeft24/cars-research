import struct, sys

def djb_hash(_str):
    _str = _str.lower()
    hash = 5381
    for x in range(len(_str)):
        hash = (((hash << 5) + hash) + ord(_str[x])) & 0xFFFFFFFF
    return hash

hash_labels = open("../xbr_hashes.txt", "r").read().split("\n")
label_dict = {}

for x in range(len(hash_labels)):
    hash = djb_hash(hash_labels[x])
    label_dict[hash] = hash_labels[x]

xbr = open("cars3.xbr", "rb")

xbr_header = struct.unpack(">IIIIII", xbr.read(24))

version = xbr_header[0]
number_files = xbr_header[1]
info_size = xbr_header[4]
zero = xbr_header[5]

xbr_hash_list = []
for x in range(number_files):
    xbr.seek(24+12*x)
    table_entry = struct.unpack(">III", xbr.read(12))
    HASH = table_entry[0]
    SIZE = table_entry[1]
    OFFSET = table_entry[2]
    xbr_hash_list.append(HASH)

while True:
    string = input()
    if string.startswith("index "):
        index = int(string.replace("index ", ""))
        if index < len(xbr_hash_list) and index >= 0:
            __hash = xbr_hash_list[index]
            try:
                name = label_dict[__hash]
            except:
                name = "0x" + hex(__hash)[2:].zfill(8)
            print(name)
    elif string.startswith("0x"):
        __hash = int(string, 16)
        try:
            name = label_dict[__hash]
        except:
            name = "Uncracked"
        try:
            index = xbr_hash_list.index(__hash)
        except:
            index = -1
        print(name, index)
    else:
        if string.startswith('"') and string.endswith('"'):
            string = string.replace('"', '')
        calculated_hash = "0x" + hex(djb_hash(string))[2:].zfill(8)
        try:
            index = xbr_hash_list.index(djb_hash(string))
        except:
            index = -1
        print(calculated_hash, index)