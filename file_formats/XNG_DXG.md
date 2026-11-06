# XNG / DXG — Static and Skinned Models

`.xng` is the model format seen on MN (PC, 360, PS3) and ROR (360, PS3), and `.dxg` is the little-endian Direct3D variant used by TVG PC. **XNG is always big-endian across all platforms, including MN PC.** Both hold a bone hierarchy, mesh names, levels of detail and per-mesh vertex streams.

| | |
|---|---|
| Byte order | XNG big-endian, DXG little-endian |
| Magic | `xng\0` |
| Tools | [`tools/xng-to-dxg`](../tools/xng-to-dxg) (convert), [`tools/scn/import-scn.py`](../tools/scn/import-scn.py) (Blender import of XNGs) |

## Other model formats

| Format | Platform | Notes |
|---|---|---|
| `.gcg` | GameCube and Wii ports | Not documented here |
| `.psg` | PS2 ports | Not documented here |
| `.p3g` | PS3 ports only | Format-wise 1:1 with XNG, so everything below applies unchanged |

## Layout

| Section | Description |
|---|---|
| Magic `xng\0`, `i32` version, `u32` bone count | |
| Bones | 128-byte name; 4×4 matrix (16 × f32); bounding-box center (3 × f32); half extents (3 × f32); radius (f32); `u32` parent index |
| `i32` mesh-name count, then 64-byte names | |
| `u8` LOD count, `u8` skin-animates flag, `u8` has-weight flag, `u8` unused | |
| LODs | `f32 auto_lod_value`, `u32 mesh_count`, then the meshes |
| Matrix palettes | Present only for skinned models: **complex** palettes when bone count ≥ 29, **simple** palettes when 1 < bone count < 29 |

(`import-scn.py` reads some of these fields under different names; the layout above follows the Rust definitions in `xng.rs`.)

## Mesh

| Type | Description |
|---|---|
| u32 | Surface index |
| u32 | Vertex type bitfield (below) |
| f32[8] | Compression constants, only if bit `0x2000` is set |
| u8 | Number of texture-coordinate sets |
| u8 × 5 | `compressed`, `streaming`, and three unknown flags |
| f32[n] | Texture-coordinate set values |
| u16 | Vertex count |
| u16 | Face-index count |
| u16[] | Triangle-strip/list indices |
| u16 | Extra pad value, only if the index count is odd |
| streams | One array per set bit in the vertex type, in the order below |

### Vertex type bits and streams (in file order)

| Bit | Stream |
|---|---|
| `0x01` | Positions (3 × f32) |
| `0x02` | Normals (3 × f32) |
| `0x08` | Vertex colors (u32, ARGB) |
| `0x04` | UV set 1 (2 × f32) |
| `0x40` | Single-bone weights (f32) |
| `0x1000` | Bone indices (4 × f32), then bone weights (4 × f32) |
| `0x10` | UV set 2 |
| `0x4000` | UV set 3 |
| `0x8000` | UV set 4 |
| `0x20` | Tangents (3 × f32) |
| `0x100` | Delta (morph target) block |
| `0x2000` | Compressed vertices |

Delta block: channel count, 64-byte controller name, vertex count, `xyz_bits`, flags, delta positions/normals/indices, then base positions and normals.

## XNG → DXG conversion

`xng-to-dxg` is **only** for converting XNG models from MN PC, MN 360, MN PS3, ROR 360 and ROR PS3 to DXG for use in TVG PC. It removes tangents (`0x20`), delta data (`0x100`), UV sets 3 and 4, and compression, then rewrites the meshes little-endian with a Direct3D vertex declaration (`D3DVSDT_*` stream types). Compressed XNGs (`0x2000`) are not supported yet and the tool stops with a panic.

## Coordinate system

The games use a left-handed, Y-up system. `import-scn.py` converts to Blender by swapping Y and Z.
