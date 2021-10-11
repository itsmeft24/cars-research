import sys, struct
from math import fmod

def c_int_mod(x, y):
    return int(fmod(x, y))

def pad_num(num, pad_to=4):
    if c_int_mod(num, pad_to) != 0:
        return ((num // pad_to) + 1)*pad_to
    else:
        return num

def get_padded_size(width, height, blk_width, blk_height, bpp):
    raw_size = pad_num(width, blk_width) * pad_num(height, blk_height)
    return round(raw_size * (bpp/8))

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

image_size = 0

if pixelformat == 0x3A:
    print("PIXELFORMAT: CI8")
    number_colors = int.from_bytes(gct.read(4), "big")
    gct.seek(0x20C)
    num_images = int.from_bytes(gct.read(4), "big") # number of mipmaps
    wh = struct.unpack(">II", gct.read(8))
    width = wh[0]
    height = wh[1]
    image_size = get_padded_size(width, height, 8, 4, 8)
    if num_images > 1:
        gct.seek(gct_size-image_size)
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
        plt_buf.write((len(plt)+0x40).to_bytes(4, byteorder='big'))
        plt_buf.write(b"\x00\x00\x00\x01\x00\x00\x00\x00")
        plt_buf.write(b"\x00\x00\x00\x40")
        plt_buf.write(b"\x00\x00\x00\x00") # Name offset is set to 0 so BrawlBox will ignore it
        plt_buf.write(b"\x00\x00\x00\x02")
        plt_buf.write(number_colors.to_bytes(2, byteorder='big')+b"\x00\x00")
        plt_buf.seek(0x40)
        plt_buf.write(plt)
        
    else:
        pass
    num_images = 1
elif pixelformat == 0x3C:
    print("PIXELFORMAT: I8")
    gct.seek(0xC)
    num_images = int.from_bytes(gct.read(4), "big") # number of mipmaps
    wh = struct.unpack(">IIII", gct.read(16))
    width = wh[0]
    height = wh[1]
    mipmin_width = wh[2]
    mipmin_height = wh[3]
    image_size = get_padded_size(width, height, 8, 4, 8)
    if num_images > 1:
        gct.seek(gct_size-image_size)
    else:
        gct.seek(0x24)
    data = gct.read()
    image_format = 0x01 # I8
    has_palette = 0
    num_images = 1
elif pixelformat == 0x29:
    print("PIXELFORMAT: CMPR")
    gct.seek(0xC)
    num_images = int.from_bytes(gct.read(4), "big") # number of mipmaps
    wh = struct.unpack(">IIII", gct.read(16))
    width = wh[0]
    height = wh[1]
    mipmin_width = wh[2]
    mipmin_height = wh[3]
    image_size = get_padded_size(width, height, 8, 8, 4)
    if num_images > 1:
        gct.seek(gct_size-image_size)
    else:
        gct.seek(0x24)
    print(gct.tell())
    data = gct.read()
    print(len(data))
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
    image_size = get_padded_size(width, height, 4, 4, 32)
    if num_images > 1:
        gct.seek(gct_size-image_size)
    else:
        gct.seek(0x24)
    data = gct.read()
    image_format = 0x06 # RGBA8
    has_palette = 0
    num_images = 1
else:
    print("Unsupported Pixelformat! Please DM this file to @itsmeft24#5576 on Discord!")
    exit(-1)
string_offset = 0
# Write TEX0
tex0.write(magic)
tex0.write(struct.pack(">IIIIIIHHII", image_size+0x40, version_number, brres_offset, section_offset, string_offset, has_palette, width, height, image_format, num_images))
tex0.seek(0x40)
tex0.write(data)
