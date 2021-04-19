import os, sys
from PIL import Image
tex0 = open(sys.argv[2], "wb")
gct = open(sys.argv[1], "rb")

gct.seek(0x4)
pixelformat = int.from_bytes(gct.read(4), "big")

num_images = 1

unknown_value = b"\x00\x00\x00\x00"

magic = b"TEX0"
version_number = 1
brres_offset = 0
section_offset = 0x40
string_offset = 0



if pixelformat == 0x3A:
    print("PIXELFORMAT: CI8")
    gct.seek(0x210)
    width = int.from_bytes(gct.read(4), "big")
    gct.seek(0x214)
    height = int.from_bytes(gct.read(4), "big")
    gct.seek(0x224)
    data = gct.read()
    image_format = 0x09 # CI8
    has_palette = 1
    print("Generating PLT0...")
    gct.seek(0x10)
    plt = gct.read(0x200)#.to_bytes(0x200, byteorder='big')
    plt_buf = open("out.plt0", "wb")
    plt_buf.write(b"\x50\x4C\x54\x30\x00\x00\x02\x40\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x02\x00\xC0\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00")
    

    #plt = "".join(map(str.__add__, plt[-2::-2] ,plt[-1::-2]))
    
    plt_buf.write(plt)#[::-1])
    # plt_buf.write((sys.getsizeof(plt)+0x40).to_bytes(4, byteorder='big'))
    # plt_buf.write(bytes(0x00000001))
    # plt_buf.write()
    
elif pixelformat == 0x29:
    print("PIXELFORMAT: CMPR")
    gct.seek(0x10)
    width = int.from_bytes(gct.read(4), "big")
    gct.seek(0x14)
    height = int.from_bytes(gct.read(4), "big")
    gct.seek(0x24)
    data = gct.read()
    image_format = 0x0e # CMPR
    has_palette = 0
else:
    print("Unsupported Pixelformat! Please DM this file to @data.arc#5576 on Discord!")
    exit(-1)

size = sys.getsizeof(data) + 0xB


# TEX0 Header
tex0.write(magic)
tex0.write(size.to_bytes(4, byteorder='big'))
tex0.write(version_number.to_bytes(4, byteorder='big'))
tex0.write(brres_offset.to_bytes(4, byteorder='big'))
tex0.write(section_offset.to_bytes(4, byteorder='big'))
tex0.write(string_offset.to_bytes(4, byteorder='big'))
# Image Header
tex0.write(has_palette.to_bytes(4, byteorder='big'))
tex0.write(width.to_bytes(2, byteorder='big'))
tex0.write(height.to_bytes(2, byteorder='big'))
tex0.write(image_format.to_bytes(4, byteorder='big'))
tex0.write(num_images.to_bytes(4, byteorder='big'))
tex0.write(unknown_value*6)
tex0.write(data)
