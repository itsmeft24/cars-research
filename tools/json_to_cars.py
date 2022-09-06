import json, struct, sys, os

if len(sys.argv) < 4:
    print(".JSON to Cars Localization (.BIN and .LOC) Converter\n")
    print("Usage: python json_to_cars.py <INPUT_JSON> <OUTPUT_BIN> <OUTPUT_LOC> <Flags>\n")
    print("Flags:")
    print("\t-utf8XXX - (Encode string values that are just a bunch of X's in utf-8 instead of utf-16.)")
    exit(-1)

json_file = open(sys.argv[1], "r")
json_dict = json.loads(json_file.read())

values = []
textids = []
for entry in json_dict:
    textids.append(entry["TextID"])
    values.append(entry["Value"])
    if "CopiedToEnd" in entry:
        values.append(entry["CopiedToEnd"])

value_str_table = b''
textid_str_table = b'\x00'.join(x.encode() for x in textids) + b'\x00'

if len(sys.argv) < 5:
    value_str_table = b'\x00\x00'.join(x.encode("utf-16").replace(b'\xFF\xFE', b'') for x in values) + b'\x00\x00'
elif sys.argv[4] == "-utf8XXX":
    value_str_table = b'\x00\x00'.join(x.encode("utf-16").replace(b'\xFF\xFE', b'') if x != len(x) * "X" else x.encode() for x in values) + b'\x00\x00'
else:
    print("Invalid flag. Must be either '-utf8XXX' or nothing")

# BIN WRITING

BIN = open(sys.argv[2], "wb")
BIN.write(struct.pack("<IH", len(value_str_table) + 2, 0xFEFF))
BIN.write(value_str_table)

for entry in json_dict:
    is_dynamic = entry["IsDynamic"]
    
    BIN.write(struct.pack("<b", is_dynamic))
    
    if "Appends" in entry:
        BIN.write(struct.pack("<b", len(entry["Appends"])))
        for appension_index in entry["Appends"]:
            BIN.write(struct.pack("<I", appension_index))
        for position in entry["AtPositions"]:
            BIN.write(struct.pack("<H", position))
    else:
        BIN.write(struct.pack("<b", 0))

    if "IsAppendedTo" in entry:
        BIN.write(struct.pack("<b", len(entry["IsAppendedTo"])))
        for appended_to in entry["IsAppendedTo"]:
            BIN.write(struct.pack("<I", appended_to))
    else:
        BIN.write(struct.pack("<b", 0))

BIN.close()

# LOC WRITING

LOC = open(sys.argv[3], "wb")

LOC.write(struct.pack("<II", len(textids), len(textid_str_table)))
LOC.write(textid_str_table)

for entry in json_dict:
    LOC.write(struct.pack("<H", entry["MaxSize"]))

LOC.close()