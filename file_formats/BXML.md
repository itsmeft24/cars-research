# BXML — Binary XML

Binary form of the games' XML data files. [`tools/bxml`](../tools/bxml) (`XMLTool`) disassembles BXML to XML and assembles it back.

| | |
|---|---|
| Magic | `BXML` |
| Version | `1002` (the tool was updated from `1000`) |
| Byte order | Little-endian (structures are read directly with `fread`) |
| Tool | `XMLTool -disasm <in.bxml> <out.xml>` / `-asm <in.xml> <out.bxml>` (Windows-only, uses `comutil.h`) |

## File layout

Header → string heap → binary value heap → attribute table → tag table.

### Header (32 bytes, eight `i32`)

| Field | Description |
|---|---|
| `BXML` | Magic `"BXML"` |
| `Version` | `1002` |
| `NumStrings` | Number of null-terminated strings in the heap |
| `StringSize` | Size of the string heap in bytes |
| `BinarySize` | Size of the binary value heap in bytes |
| `NumAttribs` | Number of attribute entries |
| `NumTags` | Number of tag entries |
| `EndOfFileOffset` | Offset of end of file |

### Tag entry (36 bytes)

| Type | Field | Description |
|---|---|---|
| i32 | `NameStringIndex` | Index into the string list |
| i32 | `ValueIndex` | String index for string values, `-1` for none, otherwise a byte offset into the binary heap |
| i32 | `ValueLen` | Byte count for binary, element count for arrays, `0` for string/none |
| i16 | `ValueType` | See below |
| i16 | unknown | |
| i32 | `FirstChildIndex` | |
| i32 | `NumChildren` | |
| i32 | `FirstAttribIndex` | |
| i32 | `NumAttribs` | |

### Attribute entry (12 bytes)

| Type | Field |
|---|---|
| i32 | `NameStringIndex` |
| i32 | `ValueIndex` |
| u16 | `ValueLen` |
| u16 | `ValueType` |

### Value types

`0` none, `1` string, and then binary, int, uint, float, double, vector4 (4 × f32), matrix4x4 (16 × f32) and bool. Non-string values are written in the XML as text with a `_valuetype="…"` attribute so they survive a round trip.

## Notes

- The numeric IDs for the types after `string` are defined in `XMLTool.h`; check there before relying on them.
- Child and attribute lists are stored as ranges into the flat tag and attribute tables.
