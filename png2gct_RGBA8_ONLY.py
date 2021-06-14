from PIL import Image
import numpy as np
import io, sys, struct

RGBA8_HEADER = b"\x00\x00\x00\x02\x00\x00\x00\x0F\x00\x00\x00\x00"

#Sussy shit. Instead of padding the blocks like a normal person would, I simply make the image always be evenly divided by blocks. Small brain play, I know.
def tex_encode_rgba8(image):
    output = io.BytesIO()
    width, height = image.size
    blk_width = 4
    blk_height = 4
    height = height + (height%4)
    width = width + (width%4)
    padded = Image.new("RGBA", (width, height), color=0)
    padded.paste(image, (0,0))
    image = np.array(padded)
    del padded
    for row in np.arange(height - blk_height + 1, step=blk_height):
        for col in np.arange(width - blk_width + 1, step=blk_width):
            #blk_padded = np.zeros((4, 4, 4), dtype=np.uint8)
            #blk = image[row:row+blk_height, col:col+blk_width]#.flatten(order='C')
            #blk_padded[:blk.shape[0], :blk.shape[1], :blk.shape[2]] = blk
            #blk_padded = blk_padded.flatten(order='C')
            blk = image[row:row+blk_height, col:col+blk_width].flatten(order='C')
            A = blk[3::4].flatten(order='C')
            R = blk[0::4].flatten(order='C')
            G = blk[1::4].flatten(order='C')
            B = blk[2::4].flatten(order='C')
            AR = np.empty((A.size + R.size,), dtype=np.uint8)
            AR[0::2] = A
            AR[1::2] = R
            GB = np.empty((G.size + B.size,), dtype=np.uint8)
            GB[0::2] = G
            GB[1::2] = B
            output.write(AR)
            output.write(GB)
    output.seek(0)
    return output.read()

image = Image.open(sys.argv[1]).convert("RGBA")

width, height = image.size
image_size = width*height*4
number_images = 1

gct = open(sys.argv[2], "wb")

gct.write(RGBA8_HEADER)
gct.write(struct.pack(">IIIIII", number_images, width, height, width, height, image_size))
gct.write(tex_encode_rgba8(image))
