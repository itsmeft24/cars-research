# GCT — GameCube/Wii Texture

Texture container used by the Wii versions of the games. The pixel data is raw GX texture data (big-endian), so it converts to Wii TEX0/PLT0 files without recompression.

| | |
|---|---|
| Byte order | Big-endian |
| Tools | [`tools/tex/`](../tools/tex): `gct2tex0.py`, `tex02gct.py`, `png2gct.py` |

## Header

| Offset | Type | Description |
|---|---|---|
| 0x00 | u32 | Always `2` |
| 0x04 | u32 | GCT pixel format (below) |
| 0x08 | u32 | `0` for non-paletted formats |

### Formats

| GCT ID | Format | TEX0 ID | Block size (w × h) | Bits per pixel |
|---|---|---|---|---|
| `0x29` | CMPR (DXT1-like) | `0x0E` | 8 × 8 | 4 |
| `0x0F` | RGBA8 | `0x06` | 4 × 4 | 32 |
| `0x3C` | I8 | `0x01` | 8 × 4 | 8 |
| `0x3A` | CI8 (palette) | `0x09` | 8 × 4 | 8 |

Width and height are padded up to the block size.

## Non-paletted layout (`0x29`, `0x0F`, `0x3C`)

| Offset | Type | Description |
|---|---|---|
| 0x0C | u32 | Image count (mipmap levels) |
| 0x10 | u32 | Width |
| 0x14 | u32 | Height |
| 0x18 | u32 | Smallest mip width |
| 0x1C | u32 | Smallest mip height |
| 0x20 | u32 | Size of image data |
| 0x24 | … | Pixel data |

## CI8 layout (`0x3A`)

| Offset | Type | Description |
|---|---|---|
| 0x08 | u32 | Number of palette colors |
| 0x0C | u16[…] | Palette (RGB5A3 as stored by the console) |
| 0x20C | u32 | Image count |
| 0x210 | u32 × 2 | Width, height |
| 0x218 | u32 × 2 | Smallest mip width, height |
| 0x220 | u32 | Size of image data |
| 0x224 | … | Pixel data |

`gct2tex0.py` assumes the CI8 palette area occupies `0x0C–0x20C` (256 colors) regardless of the stated count, and `tex02gct.py` now zero-pads the palette it writes to 256 entries so both agree.

## Mipmaps

When the image count is greater than 1, `gct2tex0.py` takes the base image from the **last** `image_size` bytes of the file. It does not carry mip levels over to TEX0.

## Conversion notes

- `png2gct.py` only writes RGBA8. It pads the image by pasting it onto a canvas rounded up to the 4-pixel block size, then interleaves the AR and GB bytes per 4×4 block as GX requires.
- `gct2tex0.py -P` also writes a PLT0 palette file for CI8 images (the TEX0/PLT0 files can be opened in BrawlBox).
