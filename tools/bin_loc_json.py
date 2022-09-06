import struct, json, sys, os

if len(sys.argv) < 4:
    print("Cars Localization (.BIN and .LOC) to .JSON Converter\n")
    print("Usage: python bin_loc_json.py <INPUT_BIN> <INPUT_LOC> <OUTPUT_JSON>")
    exit(-1)

loc = open(sys.argv[1], "rb")
bin = open(sys.argv[2], "rb")

bin_size = struct.unpack("<IH", bin.read(6))[0] # BOM follows the bin_size member.

pos = bin.tell()
curr_str = b""
strings = []
while pos < bin_size + 4:
    read_byte = bin.read(0x2)
    if read_byte == b"\x00\x00":
        strings.append(curr_str.decode("UTF-16"))
        curr_str = b""
    else:
        curr_str = curr_str + read_byte
    pos = bin.tell()

loc_header = struct.unpack("<II", loc.read(8))

num_strings = loc_header[0]
size_labels = loc_header[1]

labels = loc.read(size_labels).split(b"\x00")[:-1]
sizes = []
for x in range(num_strings):
    sizes.append(struct.unpack("<H", loc.read(2))[0])
    labels[x] = labels[x].decode()

out_json = []

bin_str_index = 0

for loc_str_index in range(num_strings):

    entry = {}
    entry['TextID'] = labels[loc_str_index]
    entry['Value'] = strings[bin_str_index]
    entry['MaxSize'] = sizes[loc_str_index]
    
    entry['IsDynamic'] = int.from_bytes(bin.read(1), 'little') 
    
    local_125 = int.from_bytes(bin.read(1), 'little')
    
    if local_125 != 0:
        print("entry:" + labels[loc_str_index] + " is appended to :"+str(local_125)+ " strings:")
        
        entry['Appends'] = []
        for y in range(local_125):
            indx = int.from_bytes(bin.read(4), 'little')
            print(indx)
            entry['Appends'].append(
                indx
            )
            
        entry['AtPositions'] = []
        for y in range(local_125):
            entry['AtPositions'].append(
                int.from_bytes(bin.read(2), 'little')
            )
        
        entry["CopiedToEnd"] = strings[bin_str_index + 1]
        
        bin_str_index+=1
    
    pbVar12 = int.from_bytes(bin.read(1), 'little')
    
    if pbVar12 != 0:
        print("entry:" + labels[loc_str_index] + " has pbVar12 :"+str(pbVar12))
        
        entry['IsAppendedTo'] = []
        for y in range(pbVar12):
            indx = int.from_bytes(bin.read(4), 'little')
            print(indx)
            entry['IsAppendedTo'].append(
                indx
            )
    
    out_json.append(entry)
    
    bin_str_index+=1

json_out = open(sys.argv[3], "w")
json_out.write(json.dumps(out_json, indent = 3))
