# COL — Collision (`.gol`, `.grid.col`)

Two collision formats appear in scene and character folders (`C\Scenes\<scene>\Col\…`). Both are documented from Blender import scripts in [`tools/col`](../tools/col), and the coverage is partial.

## `.gol` — object collision (Race-O-Rama, big-endian)

Read by `import-col.py`. Used for scene walls (`COLLWALL1.gol`), `World_Collision_*.gol`, and character collision (`Chars\McQ\Col\McQF.gol`).

| Section | Layout |
|---|---|
| File header (16 bytes) | `i32`, `char[4]`, `i32`, `i32` (ids/counts; the 4-byte tag is not named yet) |
| Object header (72 bytes) | `i32`, 12 × `f32` (likely transform/bounds), then 5 × `i32` |
| Vertices | `i16 × 3` (quantised) or `f32 × 3` |
| Normals | Present when a `i32` flag is non-zero |
| BSP-like tree | Branch nodes (32 bytes: `f32`, 3 × `f32`, 3 × `f32`, 2 × `u16`) and leaf nodes (22 bytes: `f32`, 3 × `f32`, 3 × `i16`) |

Branch nodes reference children by index; a child index with bit `0x8000` set refers to a leaf.

## `.grid.col` — terrain grid collision (Mater-National, little-endian)

Read by `import-grid-col.py` and written by `export-col.py`. It is a tagged stream: each section starts with a one-byte ID.

| Section | Layout |
|---|---|
| Header (29 bytes) | `i8 id, f32 version, i8 id, i32 flags, i8 id, i32 file_extension_offset, i8 id, f32 max_depth, f32 max_height, i8 id, i32 terrain_record_count` |
| Records | `i8 id, i8 faces_id, i32 face_count`, then faces as 3 × `i32` indices |
| Vertices | `i8 id, i32 count`, then 3 × `f32` each |
| Channel data | `i8 id, i8 bitfield` |
| Record index entry (20 bytes) | `i32 file_offset, f32 min_x, f32 min_y, f32 max_x, f32 max_y` |
| Tree node (9 bytes) | `i8 dimension, f32 min, f32 max` |

`export-col.py` writes a header with version `2.1`, a file-extension offset of `0x5000`, and named records (`polygon0`, `polygon1`, …).

## Caveats

- The scripts have hard-coded paths and are experiments, not finished tools.
