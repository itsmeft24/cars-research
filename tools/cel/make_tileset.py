import numpy as np
import argparse, struct, shutil, math, os, sys
import tcod.tileset

from typing import Any, Optional
from numpy.typing import NDArray
from PIL import Image, ImageDraw, ImageFont

CharactersString = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[\\]^_`{|}~ ¡¢£¥¦§¨©ª«¬­®¯°²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß÷"

CharMap = [ord(c) for c in list(CharactersString)]

square_size = np.ceil(np.sqrt(len(CharMap))).astype(int)
CHARMAP_TCOD = np.zeros(square_size * square_size, dtype=int)
CHARMAP_TCOD[:len(CharMap)] = np.asarray(CharMap, dtype=int)
CHARMAP_TCOD = CHARMAP_TCOD.reshape(square_size,square_size)
print(CHARMAP_TCOD)

cel = open("C:\\Projects\\Cars2\\Development-Upstream\\Mods\\NeutraDisplay\\E\\Font\\font_title_NeutraDisplayTitling52.cel", "wb")

def round_up(x):
    return 4 * math.ceil(x / 4)

def generate_kerning_table(font, c):
    offsets = []
    for second_char in CharactersString:
        if c != second_char:
            offset = (font.getlength(c) + font.getlength(second_char)) - font.getlength(c + second_char)
            offsets.append((second_char, offset))
    return offsets

def render(charmap: NDArray[Any], font: ImageFont, width_px: int, height_px: int, stroke_width: int) -> Image:
    """Render a tileset image using pillow.

    Args:
        charmap (np.ndarray): A 2D grid of codepoints to render.
        font (ImageFont): The font to be rendered.
        width_px (int): The pixel width of each rendered glyph.
        height_px (int): The pixel height of each rendered glyph.

    Returns:
        Image: A rendered tileset with the shape and characters of charmap and the glyphs of font.
    """
    IMAGE_H_CELLS, IMAGE_W_CELLS = charmap.shape
    TEXTURE_SIZE = round_up(IMAGE_W_CELLS * (width_px + 2 * stroke_width)), round_up(IMAGE_H_CELLS * (height_px + 2 * stroke_width))
    print(TEXTURE_SIZE)
    canvas = Image.new("RGBA", TEXTURE_SIZE)
    draw = ImageDraw.Draw(canvas)
    
    curr_xstart = 0
    curr_ystart = 0
    
    for cell_y, row in enumerate(charmap):
        for cell_x, char in enumerate(row):
            if char == 0:
                continue
            
            left,top,right,bottom = font.getbbox(chr(char), stroke_width=0)
            height = bottom - top
            width = right - left
            
            left_bold,top_bold,right_bold,bottom_bold = font.getbbox(chr(char), stroke_width=stroke_width)
            height_bold = bottom_bold - top_bold
            width_bold = right_bold - left_bold
            
            if width != 0 and height != 0:
                top_part = np.linspace(np.array([255,255,255]), np.array([255,255,255]), int(round(10/24 * bottom)), True).astype(np.uint8)
                gradient = np.linspace(np.array([255,255,255]), np.array([0x92,0x92,0x92]), int(round(8/24 * bottom)), True).astype(np.uint8)
                bottom_part = np.linspace(np.array([0x92,0x92,0x92]), np.array([0x92,0x92,0x92]), int(round(6/24 * bottom)), True).astype(np.uint8)
                gradient = np.concatenate((np.concatenate((top_part, gradient), axis = 0), bottom_part), axis = 0)
                gradient = np.tile(gradient, [1 * (right), 1, 1]).swapaxes(0,1)
                gradient = Image.fromarray(gradient)
                gradient = gradient.resize((right, bottom))
                
                mask = Image.new('RGBA', (right, bottom))
                draw_mask = ImageDraw.Draw(mask)
                draw_mask.text((0, 0), chr(char), fill="white", font=font)
                draw.text((curr_xstart, curr_ystart), chr(char), font=font, stroke_width=stroke_width, stroke_fill=(0x31, 0x31, 0x31))
                canvas.paste(gradient, (curr_xstart, curr_ystart), mask=mask)
                #draw.text(((width_px + stroke_width) * cell_x + stroke_width, (height_px + stroke_width) * cell_y + stroke_width), chr(char), fill="white", font=font)
            
            x_start = (curr_xstart+left_bold) / TEXTURE_SIZE[0]
            y_start = (curr_ystart+min(0, top_bold)) / TEXTURE_SIZE[1]
            x_end = (curr_xstart+right_bold) / TEXTURE_SIZE[0]
            y_end = (curr_ystart+height_px) / TEXTURE_SIZE[1]
            
            cel.write(struct.pack("<iffff", char, x_start, y_start, x_end, y_end))
            cel.write(struct.pack("<i5h", char, width_bold, -4, 0, height_px - min(0, top_bold), 0))
            
            curr_xstart += width_px + 7
        curr_ystart += height_px + 10
        curr_xstart = 0
    return canvas


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input_file", type=str, help="A truetype (.ttf) font file.")
    # parser.add_argument("--height", type=int, help="Height of each glyph (bounding box) in pixels.", required=True)
    parser.add_argument("-o", "--out", type=str, help="The output file. (Defaults to <input>.png)", default=None)
    args = parser.parse_args()

    out_file: Optional[str] = args.out
    if out_file is None:  # Replace the input file extension with png by default.
        out_file = args.input_file.rsplit(".", 1)[0] + ".png"

    tile_height: float = round(0.75 * 52)
    font = ImageFont.truetype(args.input_file, size=tile_height)
    tile_width = -1
    
    atlas_size = [0, 0]
    for c in CharactersString:
        tile_width = max(tile_width, max(font.getbbox(c, stroke_width=3)[2] - font.getbbox(c, stroke_width=3)[0], font.getbbox(c, stroke_width=3)[2] - abs(font.getbbox(c, stroke_width=3)[0])))
        tile_height = max(tile_height, max(font.getbbox(c, stroke_width=3)[3] - font.getbbox(c, stroke_width=3)[1], font.getbbox(c, stroke_width=3)[3] - abs(font.getbbox(c, stroke_width=3)[1])))
    
    atlas_size[0] = -1
    for row in CHARMAP_TCOD:
        max_height = -1
        row_width = 0
        for c in row:
            row_width += font.getbbox(chr(c), stroke_width=3)[2]
            bottom = font.getbbox(chr(c), stroke_width=3)[3]
        if row_width > atlas_size[0]:
            atlas_size[0] = row_width
    atlas_size[1] = tile_height * len(CHARMAP_TCOD)
    
    stroke_width = 3
    TEXTURE_SIZE = round_up(CHARMAP_TCOD.shape[0] * (tile_width + 2 * stroke_width)), round_up(CHARMAP_TCOD.shape[1] * (tile_height + 2 * stroke_width))
    
    cel.write(struct.pack("<f2hI", 2.8, 0, 0, len(CharactersString)))
    render(CHARMAP_TCOD, font, tile_width, tile_height, stroke_width).save(out_file)
    
    os.system('texconv -m 1 -y -f BC3_UNORM "' + out_file + '"')
    shutil.copyfile(out_file.replace(".png", ".dds"), "C:\\Projects\\Cars2\\Development-Upstream\\Mods\\NeutraDisplay\\E\\Font\\font_title_NeutraDisplayTitling52.dds")


if __name__ == "__main__":
    main()