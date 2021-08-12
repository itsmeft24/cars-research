import os, sys, struct

if len(sys.argv) < 3:
    print(".TEX0 to Cars .GCT\n\nUsage: python tex02gct.py <INPUT_TEX0> <OUTPUT_GCT> <INPUT_PLT0>\n\nIf your image is in CI8, then you need to specify a palette file. Otherwise, you can skip the INPUT_PLT0 argument.")
    exit()
tex0 = open(sys.argv[1], "rb")
gct = open(sys.argv[2], "wb")

CMPR_HEADER = b"\x00\x00\x00\x02\x00\x00\x00\x29\x00\x00\x00\x00"
RGBA8_HEADER = b"\x00\x00\x00\x02\x00\x00\x00\x0F\x00\x00\x00\x00"
CI8_HEADER =  b"\x00\x00\x00\x02\x00\x00\x00\x3A"

tex0_header = struct.unpack(">4sIIIIIIHHII", tex0.read(0x28))

magic = tex0_header[0]
size = tex0_header[1]
version_number = tex0_header[2]
brres_offset = tex0_header[3]
section_offset = tex0_header[4]
string_offset = tex0_header[5]
has_palette = tex0_header[6]
width = tex0_header[7]
height = tex0_header[8]
pixelformat = tex0_header[9]
number_images = tex0_header[10]

if magic != b"TEX0":
    print("Invalid TEX0 File.")
    exit(-1)

if pixelformat == 0x0e:
    tex0.seek(0x40)
    image_size = int((width*height)/2)
    data = tex0.read(image_size)
    gct.write(CMPR_HEADER)
    number_images = 1
    gct.write(struct.pack(">IIIIII", number_images, width, height, width, height, image_size))
    gct.write(data)
elif pixelformat == 0x06:
    tex0.seek(0x40)
    image_size = width*height*4
    data = tex0.read(image_size)
    gct.write(RGBA8_HEADER)
    number_images = 1
    gct.write(struct.pack(">IIIIII", number_images, width, height, width, height, image_size))
    gct.write(data)
elif pixelformat == 0x09:
    if len(sys.argv) < 4:
        print("Please specify an input palette file!")
        exit(-1)
    else:
        palette = open(sys.argv[3], "rb")
    tex0.seek(0x40)
    image_size = width*height
    data = tex0.read(image_size)
    palette.seek(0x1c)
    number_colors = int.from_bytes(palette.read(2), "big")
    palette.seek(0x40)
    palette = palette.read(number_colors*2)
    gct.write(CI8_HEADER)
    gct.write(number_colors.to_bytes(4, byteorder='big'))
    gct.write(palette)
    number_images = 1
    gct.write(struct.pack(">IIIIII", number_images, width, height, width, height, image_size))
    gct.write(data)
else:
    print("Unsupported Pixelformat!")
    exit(-1)
