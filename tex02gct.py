import os, sys, struct

if len(sys.argv) < 3:
    print(".TEX0 to Cars .GCT\n\nUsage: python tex02gct.py <INPUT_TEX0> <OUTPUT_GCT> <INPUT_PLT0>\n\nIf your image is in CI8, then you need to specify a palette file. Otherwise, you can skip the INPUT_PLT0 argument.")
    exit()
tex0 = open(sys.argv[1], "rb")
gct = open(sys.argv[2], "wb")

CMPR_HEADER = b"\x00\x00\x00\x02\x00\x00\x00\x29\x00\x00\x00\x00"
RGBA8_HEADER = b"\x00\x00\x00\x02\x00\x00\x00\x0F\x00\x00\x00\x00"
CI8_HEADER =  b"\x00\x00\x00\x02\x00\x00\x00\x3A"

tex0.seek(0x20)
pixelformat = int.from_bytes(tex0.read(4), "big")

if pixelformat == 0x0e:
    tex0.seek(0x8)
    version_number = int.from_bytes(tex0.read(4), "big")
    tex0.seek(0x24)
    number_images = int.from_bytes(tex0.read(4), "big")
    tex0.seek((version_number*4)+0x18)
    width = int.from_bytes(tex0.read(2), "big")
    tex0.seek((version_number*4)+0x1a)
    height = int.from_bytes(tex0.read(2), "big")
    tex0.seek(0x40)
    data = tex0.read()
    gct.write(CMPR_HEADER)
    area = int((width*height)/2)
    gct.write(number_images.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(area.to_bytes(4, byteorder='big'))
    gct.write(data)
elif pixelformat == 0x06:
    tex0.seek(0x8)
    version_number = int.from_bytes(tex0.read(4), "big")
    tex0.seek(0x24)
    number_images = int.from_bytes(tex0.read(4), "big")
    tex0.seek((version_number*4)+0x18)
    width = int.from_bytes(tex0.read(2), "big")
    tex0.seek((version_number*4)+0x1a)
    height = int.from_bytes(tex0.read(2), "big")
    tex0.seek(0x40)
    data = tex0.read()
    gct.write(RGBA8_HEADER)
    area = int((width*height)/2)
    gct.write(number_images.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(area.to_bytes(4, byteorder='big'))
    gct.write(data)
elif pixelformat == 0x09:
    if len(sys.argv) < 4:
        print("Please specify an input palette file!")
        exit(-1)
    else:
        palette = open(sys.argv[3], "rb")
    tex0.seek(0x8)
    version_number = int.from_bytes(tex0.read(4), "big")
    tex0.seek((version_number*4)+0x18)
    tex0.seek(0x1c)
    wh = struct.unpack(">HH", tex0.read(4))
    width = wh[0]
    height = wh[1]
    tex0.seek(0x24)
    number_images = int.from_bytes(tex0.read(4), "big")
    tex0.seek(0x40)
    data = tex0.read()
    palette.seek(0x1c)
    number_colors = int.from_bytes(palette.read(2), "big")
    palette.seek(0x40)
    palette = palette.read(number_colors*2)
    
    gct.write(CI8_HEADER)
    gct.write(number_colors.to_bytes(4, byteorder='big'))
    gct.write(palette)
    area = int((width*height)/2)
    gct.write(number_images.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(area.to_bytes(4, byteorder='big'))
    gct.write(data)
else:
    print("Unsupported Pixelformat!")
    exit(-1)
