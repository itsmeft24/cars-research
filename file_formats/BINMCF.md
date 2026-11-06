# BINMCF — Motion Control Pack

`*.mcf.x360.bin` files bundle one motion-control file (a BPB) with a set of zlib-compressed animation files. Found in Xbox 360 Race-O-Rama archives.

| | |
|---|---|
| Byte order | Big-endian |
| Tools | [`tools/binmcf.py`](../tools/binmcf.py), [`binmcf.rs`](../tools/xbr/extract-cars-v3-archive/src/binmcf.rs) |
| Related | [BPB](BPB.md), [MOT](MOT.md) |

## Header (0x20 bytes)

| Offset | Type | Description |
|---|---|---|
| 0x00 | char[8] | Magic: `BINMCF\0\xCC` |
| 0x08 | u32 | Offset of the embedded motion-control file |
| 0x0C | u32 | Size of the motion-control file |
| 0x10 | u32 | Offset of the animation table |
| 0x14 | u32 | Number of animations |
| 0x18 | u32 | Offset of the name string table |
| 0x1C | u32 | Size of the string table |

The Python script reads the magic as a `u64` and the Rust parser as the 8-byte magic. Both skip the same bytes.

## Animation table (16 bytes per entry)

| Type | Description |
|---|---|
| u32 | Offset of the compressed animation |
| u32 | Compressed size (`0` terminates the table early) |
| u32 | Decompressed size |
| u32 | Offset of the name within the string table (null-terminated) |

Each animation is a standard zlib stream. After decompression it is a [MOT](MOT.md) animation file. Names can contain subdirectories (`Anim\Foo.got`).

## Extraction

1. Write the embedded motion-control file out as `<name>.mcf.bpb`.
2. For each table entry, inflate the data and write it to the name from the string table.
