# XBR / P3R / PSR — Resource Archives

`.xbr` (Xbox 360), `.p3r` (PS3) and `.psr` (PS2) are the games' resource archives. Two versions are documented.

| | Version 2 | Version 3 |
|---|---|---|
| Games | Mater-National, Cars: The Video Game | Race-O-Rama (Xbox 360 / PS3 / PS2) |
| Byte order | Little-endian | Big-endian (X360 / PS3) |
| File names | Stored as strings in the table | Stored as DJB-2 hashes only |
| Tools | [`create-cars-v2-archive`](../tools/xbr/create-cars-v2-archive) | [`extract-file-cars-v3-archive`](../tools/xbr/extract-file-cars-v3-archive), [`extract-cars-v3-archive`](../tools/xbr/extract-cars-v3-archive) |

File alignment is `0x800` (a disc sector) in both versions. See [`hash_labels/`](../hash_labels) for the lists used to recover v3 file names; they cover ROR on Xbox 360 and PS3 only.

## Header (0x18 bytes)

| Type | Description |
|---|---|
| u32 | Version (`2` or `3`) |
| u32 | Number of files |
| u32 | unknown (the v2 packer writes `0x34325446`, the ASCII text `FT24`) |
| u32 | unknown |
| u32 | Size of the file table |
| u32 | v3: padding. v2: size of the string data |

## Version 3 table (starts at 0x18, 12 bytes per entry)

| Type | Description |
|---|---|
| u32 | DJB-2 hash of the file path (see below) |
| u32 | File size in bytes |
| u32 | File offset. X360: offset ÷ `0x800` (shift left 11 to get the real offset). PS3: absolute offset |

Path hashing:
- **Xbox 360**: lowercase, backslash separators (`c\act\activityinfo.dat.bpb`).
- **PS3**: lowercase, forward-slash separators (`c/act/activityinfo.dat.bpb`).
- Hash: DJB-2 — `h = 5381; for c in lower(path): h = h * 33 + c` (32-bit).

Because only hashes are stored, original names come from external label lists (`hash_labels/xbr_hashes.txt` uses `\`, `p3r_hashes.txt` uses `/`).

## Version 2 table (starts at 0x18)

Entries are variable-length and are sorted by path:

| Type | Description |
|---|---|
| u32 | Length of the path string |
| char[] | Path (not null-terminated in the table) |
| u32 | File size |
| u32 | Offset ÷ `0x800` |

File data begins at the first `0x800` boundary after the table. Every file is padded to a multiple of `0x800`.

## Known contents

- `*.bpb` files are [BPB](BPB.md) parameter blocks.
- `*.mcf.x360.bin` files are [BINMCF](BINMCF.md) packs.
- Race-O-Rama's separate music and audio archives (`Cars3_Mus.Xbr`, `Cars3_Aud.Xbr`) use the same container; their names come from built-in hash tables in the Rust extractor.
