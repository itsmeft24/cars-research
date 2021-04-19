import os, sys
from PIL import Image
tex0 = open(sys.argv[1], "rb")
gct = open(sys.argv[2], "wb")

CMPR_HEADER = b"\x00\x00\x00\x02\x00\x00\x00\x29\x00\x00\x00\x00\x00\x00\x00\x01"
CI8_HEADER = b"\x00\x00\x00\x02\x00\x00\x00\x3A\x00\x00\x00\x00\x00\x00\x00\x01"

tex0.seek(0x20)
pixelformat = int.from_bytes(tex0.read(4), "big")

if pixelformat == 0x0e:
    tex0.seek(0x8)
    version_number = int.from_bytes(tex0.read(4), "big")

    tex0.seek((version_number*4)+0x18)
    width = int.from_bytes(tex0.read(2), "big")
    tex0.seek((version_number*4)+0x1a)
    height = int.from_bytes(tex0.read(2), "big")
    tex0.seek(0x40)

    if (width & (width-1) == 0) and width != 0:
        if (height & (height-1) == 0) and height != 0:
            pass
        else:
            print("Dimensions are not powers of two!")
            exit(0)
    else:
        print("Dimensions are not powers of two!")
        exit(0)

    data = tex0.read()
    gct.write(CMPR_HEADER)
    area = int((width*height)/2)
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(area.to_bytes(4, byteorder='big'))
    gct.write(data)
elif pixelformat == 0x09:
    tex0.seek(0x8)
    version_number = int.from_bytes(tex0.read(4), "big")

    tex0.seek((version_number*4)+0x18)
    width = int.from_bytes(tex0.read(2), "big")
    tex0.seek((version_number*4)+0x1a)
    height = int.from_bytes(tex0.read(2), "big")
    tex0.seek(0x40)

    if (width & (width-1) == 0) and width != 0:
        if (height & (height-1) == 0) and height != 0:
            pass
        else:
            print("Dimensions are not powers of two!")
            exit(0)
    else:
        print("Dimensions are not powers of two!")
        exit(0)

    data = tex0.read()
    gct.write(CI8_HEADER)
    area = int((width*height)/2)
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(width.to_bytes(4, byteorder='big'))
    gct.write(height.to_bytes(4, byteorder='big'))
    gct.write(area.to_bytes(4, byteorder='big'))
    gct.write(data)
else:
    print("Unsupported Pixelformat!")
    exit(-1)

