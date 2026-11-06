# BPB — Binary Parameter Block

`.bpb` files are the binary form of the games' INI-style configuration files (`ActivityInfo.dat.bpb`, `VehicleInfo.dat.bpb`, …). The text form is called a **ParameterBlock** (`.pb`).

| | |
|---|---|
| Games | Race-O-Rama (`BPBh`), Mater-National (`BPB\0`) |
| Byte order | Big-endian |
| Hash | DJB-2 (lowercased input, seed 5381, `h = h*33 + c`) |
| Tools | [`tools/bpbconv.py`](../tools/bpbconv.py) (BPB → text), [`extract-cars-v3-archive`](../tools/xbr/extract-cars-v3-archive/src/bpb.rs) (Rust port) |
| Compression | Some files are wrapped in a `ZLIBME` container (see below) |

The text form looks like:

```ini
[Parent]
Option/Child = Value
```

## ZLIBME wrapper (optional)

If the file starts with the 6 bytes `ZLIBME`, the real BPB is a zlib stream starting at offset `0x10`. Decompress it first.

## Header (0x20 bytes)

| Offset | Type | Description |
|---|---|---|
| 0x00 | char[4] | Magic: `BPBh` (Race-O-Rama) or `BPB\0` (Mater-National) |
| 0x04 | u32 | Offset of the Parent (label) definition table |
| 0x08 | u32 | Offset of the Child (option) definition table |
| 0x0C | u16 | Number of Parents |
| 0x0E | u16 | Number of Children |
| 0x10 | u32 | Offset of the Label string data |
| 0x14 | u32 | Size of the Label string data |
| 0x18 | u32 | Offset of the Value string data |
| 0x1C | u32 | Size of the Value string data |

## Parent definition (16 bytes each, starts at 0x20)

| Type | Description |
|---|---|
| u32 | DJB-2 hash of the label |
| u32 | Offset of the label string |
| u16 | Suffix flag. `0xFFFF` = none; otherwise an integer appended to the label (e.g. `Song` + `1` → `[Song1]`) |
| u16 | Number of children |
| u32 | Index of the first child in the Child table |

## Child (option) definition (16 bytes each)

| Type | Description |
|---|---|
| u32 | DJB-2 hash of the option name |
| u32 / (u8 + u24) | Label string offset. In Race-O-Rama the first byte is a type flag and the remaining 24 bits are the offset |
| u16 | Mater-National: type flag. Race-O-Rama: child suffix (`0xFFFF` = none) |
| u16 | Padding; holds an inline string if the flag says the string is stored inline |
| u32 | Value: offset into the value string data, an inline value, or the number itself |

### Value types

| Type | Mater-National flag | Race-O-Rama flag |
|---|---|---|
| String (offset into Value String data) | `0x01` | `0x00` |
| String (stored inline, right after the definition; 5 bytes or fewer) | `0x02` | `0x40` |
| Integer (4-byte big-endian) | `0x04` | `0x80` |
| Float (big-endian single) | `0x08` | `0xC0` |

Strings that are longer than 5 bytes are placed in the Value String data; shorter ones stay inline.

## Footer

- **Label string data** — every label referenced by the file, separated by null bytes.
- **Value string data** — string values, separated by null bytes.

Names are only stored as hashes in the definition tables, so converters recover them by hashing every string in the label table and matching them. Unknown hashes print as `0x…`.

## Notes

- Race-O-Rama BPBs have an extra section before the string tables, likely a lookup-acceleration structure. Little else is known about it.
- The `bpbconv.py` implementation treats the Race-O-Rama flag + string offset as `u8` + `u24`. This works in practice, but the field might really be two `u16` values.
