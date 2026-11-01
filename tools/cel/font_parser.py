import struct, random
from PIL import Image, ImageDraw

ATLAS = (612, 560)
doc = Image.new("RGBA", ATLAS)
view = ImageDraw.Draw(doc)

colors = ["ff7474", "e66cff", "6c74ff", "6cfcff", "6cffb8", "cfff6c", "ffd16c"]

with open("C:\\Projects\\Cars2\\Development-Upstream\\Mods\\NeutraDisplay\\E\\Font\\font_user_NeutraDisplayTitling40.cel", 'rb') as f:
    version, unk1, unk2, num_cells = struct.unpack('<fhhi', f.read(12))
    for _ in range(num_cells):
        # Parse ScreenTextFontCharacterCell
        character, x1, y1, x2, y2 = struct.unpack('<iffff', f.read(20))

        # Parse ScreenTextFontKerningData
        unk, unk2, unk3, unk4, unk5, kern_table_len = struct.unpack('<ihhhhh', f.read(14))
        kern_table = []
        for _ in range(kern_table_len):
            entry = struct.unpack('<hh', f.read(4))
            kern_table.append(entry)
        
        view.rectangle([x1 * ATLAS[0], y1 * ATLAS[1], x2 * ATLAS[0], y2 * ATLAS[1]], fill="#"+random.choice(colors))
        print(chr(character), unk2, unk3, unk4, unk5)
doc.save("download_borders.png")