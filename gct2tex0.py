import os, sys, struct

if len(sys.argv) < 2:
    print("Cars .GCT to .TEX0\n\nUsage: python gct2tex0.py <INPUT_GCT> <OUTPUT_TEX0> <Flags>\n\nFlags: -P\tGenerates a pallete file. Is only used when the pixelformat of the image is CI8.")
    exit()

gct = open(sys.argv[1], "rb")
tex0 = open(sys.argv[2], "wb")

gct.seek(0, 2)
gct_size = gct.tell()

gct.seek(0x4)

pixelformat = int.from_bytes(gct.read(4), "big")

magic = b"TEX0"
version_number = 1
brres_offset = 0
section_offset = 0x40

if pixelformat == 0x3A:
    print("PIXELFORMAT: CI8")
    number_colors = int.from_bytes(gct.read(4), "big")
    gct.seek(0x20C)
    num_images = int.from_bytes(gct.read(4), "big") # number of mipmaps
    wh = struct.unpack(">II", gct.read(8))
    width = wh[0]
    height = wh[1]
    if num_images > 1:
        gct.seek(gct_size-width*height, 0)
    else:
        gct.seek(0x224)
    data = gct.read()
    image_format = 0x09 # CI8
    has_palette = 1
    if len(sys.argv) != 4:
        pass
    elif sys.argv[3] == "-P":
        print("Generating PLT0...")
        gct.seek(0xC)
        plt = gct.read(number_colors*2)
        plt_buf = open(sys.argv[2][:-5]+".plt0", "wb")
        plt_buf.write(b"PLT0")
        plt_buf.write((sys.getsizeof(plt)+0x1F).to_bytes(4, byteorder='big'))
        plt_buf.write(b"\x00\x00\x00\x01\x00\x00\x00\x00")
        plt_buf.write(b"\x00\x00\x00\x40")
        plt_buf.write((sys.getsizeof(plt)+0x23).to_bytes(4, byteorder='big'))
        plt_buf.write(b"\x00\x00\x00\x02")
        plt_buf.write(number_colors.to_bytes(2, byteorder='big')+b"\x00\x00")
        plt_buf.seek(0x40)
        plt_buf.write(plt)
        
    else:
        pass
    num_images = 1

elif pixelformat == 0x29:
    print("PIXELFORMAT: CMPR")
    gct.seek(0xC)
    num_images = int.from_bytes(gct.read(4), "big") # number of mipmaps
    
    gct.seek(0x10)
    wh = struct.unpack(">IIII", gct.read(16))
    width = wh[0]
    height = wh[1]
    mipmin_width = wh[2]
    mipmin_height = wh[3]
    if num_images > 1:
        gct.seek(gct_size-round((width*height)/2), 0)
    else:
        gct.seek(0x24)
    data = gct.read()
    image_format = 0x0e # CMPR
    has_palette = 0
    num_images = 1
elif pixelformat == 0x0f:
    print("PIXELFORMAT: RGBA8")
    gct.seek(0xC)
    num_images = int.from_bytes(gct.read(4), "big") # number of mipmaps
    
    gct.seek(0x10)
    wh = struct.unpack(">IIII", gct.read(16))
    width = wh[0]
    height = wh[1]
    mipmin_width = wh[2]
    mipmin_height = wh[3]
    if num_images > 1:
        gct.seek(gct_size-(width*height*4), 0)
    else:
        gct.seek(0x24)
    data = gct.read()
    image_format = 0x06 # RGBA8
    has_palette = 0
    num_images = 1
else:
    print("Unsupported Pixelformat! Please DM this file to @itsmeft24#5576 on Discord!")
    exit(-1)
size = sys.getsizeof(data) + 0x1F
string_offset = 0
# Write TEX0
tex0.write(magic)
tex0.write(struct.pack(">IIIIIIHHII", size, version_number, brres_offset, section_offset, string_offset, has_palette, width, height, image_format, num_images))
tex0.seek(0x40)
tex0.write(data)
