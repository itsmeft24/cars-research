# MOT — Animation Data (`.mot`, `.got`, `.wot`)

Keyframe animation files used by Mater-National (version 7) and Race-O-Rama (version 8) and later. `.got` is used for global/character objects and `.wot` for world objects.

| | |
|---|---|
| Byte order | Big-endian |
| Tools | [`tools/mot/`](../tools/mot) — Blender import/export add-ons, `anim_backporter.py`, `MOTLib.py` |
| Related | [BINMCF](BINMCF.md) (packs of compressed animations) |

## Header (16 bytes)

| Type | Description |
|---|---|
| i32 | Version, stored **negative** (`-7` or `-8`) |
| u32 | MOT type. `0` is a camera animation (not supported by the Blender importer) |
| u32 | Frame count |
| u32 | Object (bone) count |

## Object name table

For each object: `u32` length (including terminator), then a null-terminated ASCII name.

## Second header (24 bytes)

| Type | Description |
|---|---|
| f32 | Duration |
| u32 | Rotation key type (below) |
| u32 | Position key type: `0` = linear, `1` = Bezier |
| u32 | Number of position tracks |
| u32 | Number of rotation tracks |
| u32 | Number of FOV tracks |

Then 4 more bytes follow. Version 8 files have an extra 4 bytes, which the backporter skips.

## Tracks

Each track begins with a small header and is followed by its keys.

- **Position / rotation track**: `u32 key_count`, `u32 object_index`.
- **FOV track**: `u32 key_count` only (it targets the camera).

### Position keys

| Type | Size | Layout |
|---|---|---|
| Linear | 16 | `f32 time, x, y, z` |
| Bezier | 40 | `f32 time, x, y, z`, tangent in (x, y, z), tangent out (x, y, z) |

### Rotation keys (all begin with `f32 time`)

| Type | Name | Size | Layout after `time` |
|---|---|---|---|
| 0 | Byte angle-axis | 8 | `i8 angle, axisX, axisY, axisZ` |
| 1 | Short angle-axis | 12 | `i16 angle, axisX, axisY, axisZ` |
| 2 | Quaternion | 20 | 4 × `f32` |
| 3 | Half quaternion | 12 | 4 × `f16` (half float) |

Angle scaling: byte keys use `127.5 / π` per radian, and short keys use `2048 / π`. Axis components are signed-normalised (`/127` or `/32767`).

### FOV keys (8 bytes)

`f32 time, f32 fov`.

## Version 8 → 7 backporting

`anim_backporter.py` rewrites the version to `-7`, converts quaternion and half-quaternion rotations to short angle-axis (type 1), and keeps byte-angle-axis (type 0) rotations unchanged. Use it to run Race-O-Rama animations in Mater-National.

## Notes

- Bezier keys are 10 floats: time, position, tangent in (floats 4–6) and tangent out (floats 7–9). `MOTLib.BezierPositionKey` previously copied the "in" tangent into "out"; that is fixed.
