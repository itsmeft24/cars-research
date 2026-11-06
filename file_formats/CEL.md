# CEL — Bitmap Font Metrics

`.cel` files describe glyph rectangles and kerning for a bitmap font atlas (`font_user_*.cel`, `font_title_*.cel`). They live next to the font texture.

| | |
|---|---|
| Byte order | Little-endian |
| Games | Cars 2 (names in the sample scripts: `NeutraDisplayTitling40`, `…52`) |
| Tools | [`tools/cel/font_parser.py`](../tools/cel/font_parser.py) (visualise glyph boxes), [`make_tileset.py`](../tools/cel/make_tileset.py) (generate from a `.ttf`) |

## Header (12 bytes)

| Type | Description |
|---|---|
| f32 | Version (`2.8` by the generator) |
| i16 | unknown |
| i16 | unknown |
| i32 | Number of cells |

## Cell (`ScreenTextFontCharacterCell`, 20 bytes + kerning)

| Type | Description |
|---|---|
| i32 | Character code point |
| f32 × 4 | Atlas rectangle `x1, y1, x2, y2`, normalised to 0–1 |

### Kerning data (14 bytes + table)

| Type | Description |
|---|---|
| i32 | Character code point (repeated) |
| i16 | Glyph width |
| i16 | unknown (the generator writes `-4`) |
| i16 | unknown (`0`) |
| i16 | Height related value (`height_px - min(0, top)`) |
| i16 | Kerning entries count |
| (i16, i16)[n] | Kerning entries |

`font_parser.py` uses a 612 × 560 atlas and draws one random-coloured box per cell.

## Notes

- Field meanings above are partly guesses. The parser prints `unk2…unk5` for experimentation, and the generator's field order doesn't match the parser's naming exactly (both read/write the same 14 bytes).
- The generator emits 2.8 as the version and does not compute the kerning table values yet beyond measuring pair offsets.
