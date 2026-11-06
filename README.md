# cars-research

Researching the *Cars* games developed by Rainbow Studios and Incinerator Studios — file-format documentation, converters, and modding utilities.

**Games covered:** *Cars: The Video Game* (TVG), *Cars: Mater-National Championship* (MN), *Cars: Race-O-Rama* (ROR) on PC, Wii, Xbox 360, PS3 and PS2 — varies by tool. Platform support is noted for each tool.

## Related projects

| Project | Use it for |
|---|---|
| [offsetting/offsetting](https://github.com/offsetting/offsetting) | Unpacking scenes and streaming packages |
| [itsmeft24/soiboy](https://github.com/itsmeft24/soiboy) | Unpacking scenes and streaming packages |
| [high-octane-dev/rre](https://github.com/high-octane-dev/rre) | Non-matching decompilation of MN PC |
| [itsmeft24/hi-octane-hook](https://github.com/itsmeft24/hi-octane-hook) | Code modding MN PC — **archived and deprecated**, see [Code modding](#code-modding) |

> [!NOTE]
> Many tools here are research-grade: some have hard-coded paths, take positional arguments with minimal validation, or are unfinished. Back up your game files before using them. Each tool's status is listed below.

## Find what you need

| I want to… | Go to |
|---|---|
| Unpack scenes or streaming packages | [offsetting](https://github.com/offsetting/offsetting) / [soiboy](https://github.com/itsmeft24/soiboy) |
| Read the MN PC decompilation | [high-octane-dev/rre](https://github.com/high-octane-dev/rre) |
| Unpack or repack a game archive (`.xbr`, `.p3r`, `.psr`) | [Archives](#archives) |
| Read or edit a config file (`.bpb`) | [Config files](#config-files) |
| Translate or edit in-game text | [Localization](#localization) |
| Convert textures | [Textures](#textures) |
| Work with animations or models in Blender | [Models, animation & scenes](#models-animation--scenes) |
| Make a custom font | [Fonts](#fonts) |
| Edit saves | [Save data](#save-data) |
| Patch the game executable / add Wii code | [Code modding](#code-modding) |
| Identify files by hash | [Hash labels](#hash-labels) |
| Understand a binary format | [File formats](#file-formats) |

## File formats

Written specs live in [`file_formats/`](file_formats).

| Format | Doc | Extensions | Status |
|---|---|---|---|
| Binary Parameter Block | [BPB](file_formats/BPB.md) | `.bpb` | Well understood |
| Resource archives | [XBR / P3R / PSR](file_formats/XBR.md) | `.xbr` `.p3r` `.psr` | Well understood |
| Motion control pack | [BINMCF](file_formats/BINMCF.md) | `.mcf.x360.bin` | Understood |
| Animation | [MOT](file_formats/MOT.md) | `.mot` `.got` `.wot` | Well understood |
| Localization | [BIN + LOC](file_formats/LOC_BIN.md) | `.bin` `.loc` | Mostly understood |
| Textures | [GCT](file_formats/GCT.md) | `.gct` | Understood |
| Models | [XNG / DXG](file_formats/XNG_DXG.md) | `.xng` `.dxg` | Mostly understood |
| Models (other platforms) | [GCG / PSG / P3G](file_formats/XNG_DXG.md#other-model-formats) | `.gcg` `.psg` `.p3g` | P3G = XNG; others not documented |
| Scenes | [SCN](file_formats/SCN.md) | `.scn` | Partial |
| Binary XML | [BXML](file_formats/BXML.md) | `.bxml` | Understood |
| Collision | [COL](file_formats/COL.md) | `.gol` `.grid.col` | Partial |
| Fonts | [CEL](file_formats/CEL.md) | `.cel` | Partial |
| Save files | [Save data](file_formats/SAVE_DATA.md) | `PSLOT*.dat` | Partial |
| Interleaved audio | [DSP → ITL](file_formats/ITL.md) | `.itl` | Notes only |

## Tools

All tools are in [`tools/`](tools). **Requirements:** Python 3 for `.py` scripts (extra packages noted), Visual Studio for the `.sln` projects, and Cargo for Rust crates.

### Archives

| Tool | What it does | Usage | Status |
|---|---|---|---|
| [`xbr/extract-file-cars-v3-archive`](tools/xbr/extract-file-cars-v3-archive) (C++) | Extract one file from a Race-O-Rama `.xbr` / `.p3r` | `<ARCHIVE> <FILE_PATH> -x360\|-ps3\|-ps2` | Working |
| [`xbr/create-cars-v2-archive`](tools/xbr/create-cars-v2-archive) (C++) | Pack a folder into a Mater-National / Cars `.xbr` / `.psr` | `<FOLDER> <OUTPUT>` | Working |
| [`xbr/extract-cars-v3-archive`](tools/xbr/extract-cars-v3-archive) (Rust) | Extract the whole Race-O-Rama archive set (main, music, audio), convert `.bpb` to text and unpack `.mcf.x360.bin` | `cargo run` — paths are hard-coded (see note) | Unfinished |
| [`binmcf.py`](tools/binmcf.py) | Unpack a BINMCF motion pack | `<BIN_MCF> <OUT_DIR>` | Working |

> [!WARNING]
> `extract-cars-v3-archive` embeds `hash_labels/xbr_hashes.txt` as its fallback label list, reads archives from the current directory, and checks exact NTSC-U file sizes. Treat it as a work in progress.

### Config files

| Tool | What it does | Usage |
|---|---|---|
| [`bpbconv.py`](tools/bpbconv.py) | Convert `.bpb` to editable text (needs `numpy`) | `<INPUT_BPB> <OUTPUT_FILE>` |
| [`bxml/`](tools/bxml) (C++, Windows) | Disassemble / assemble BXML | `-disasm\|-asm <IN> <OUT>` |

### Localization

| Tool | What it does | Usage |
|---|---|---|
| [`bin-loc/bin_loc_json.py`](tools/bin-loc/bin_loc_json.py) | Convert `.bin` + `.loc` to JSON | `<LOC> <BIN> <OUT_JSON>` |
| [`bin-loc/json_to_cars.py`](tools/bin-loc/json_to_cars.py) | Convert JSON back to `.bin` + `.loc` | `<JSON> <OUT_BIN> <OUT_LOC> [-utf8XXX]` |

### Textures

Wii `.gct` ↔ `.tex0` / `.png` (needs `Pillow` and `numpy` for PNG). Details in the [GCT spec](file_formats/GCT.md).

| Tool | What it does | Usage |
|---|---|---|
| [`tex/gct2tex0.py`](tools/tex/gct2tex0.py) | `.gct` → `.tex0` (and `.plt0` palette) | `<GCT> <TEX0> [-P]` |
| [`tex/tex02gct.py`](tools/tex/tex02gct.py) | `.tex0` → `.gct` | `<TEX0> <GCT> [PLT0]` (palette needed for CI8) |
| [`tex/png2gct.py`](tools/tex/png2gct.py) | `.png` → RGBA8 `.gct` | `<PNG> <GCT>` |

### Models, animation & scenes

| Tool | What it does | Usage |
|---|---|---|
| [`mot/`](tools/mot) | Blender add-ons to import / export animations (`MOT_IMPORT.py`, `MOT_EXPORT.py`, shared `MOTLib.py`); see its [README](tools/mot/README.MD) | Zip the three files and install in Blender |
| [`mot/anim_backporter.py`](tools/mot/anim_backporter.py) | Backport v8 (Race-O-Rama) animations to v7 (Mater-National) | `<IN> <OUT>` |
| [`xng-to-dxg/`](tools/xng-to-dxg) (Rust) | **Only** for converting `.xng` models from MN PC, MN 360, MN PS3, ROR 360 and ROR PS3 to `.dxg` for use in TVG PC (compression unsupported) | `cargo run -- <IN.xng> <OUT.dxg>` |
| [`scn/import-scn.py`](tools/scn/import-scn.py) | Blender script that imports a scene's static models | Edit the paths at the bottom, run in Blender |
| [`col/`](tools/col) | Blender scripts to import `.gol` / `.grid.col` collision and export `.grid.col` | Edit the paths at the top, run in Blender |

### Fonts

| Tool | What it does |
|---|---|
| [`cel/make_tileset.py`](tools/cel/make_tileset.py) | Render a `.ttf` into a font atlas and write the matching `.cel` (needs `tcod`, `Pillow`, `numpy`) |
| [`cel/font_parser.py`](tools/cel/font_parser.py) | Dump a `.cel` and draw its glyph boxes |

### Save data

| Tool | What it does | Status |
|---|---|---|
| [`save-data/`](tools/save-data) (C++, "Curio") | Parse a profile save. **MN PC only.** | `<DATAPC_DIR> <SAVE_FILE>` |

### Audio

| Tool | What it does | Usage |
|---|---|---|
| [`dsp2itl.py`](tools/dsp2itl.py) | Interleave two mono DSP channels into one file | `<LEFT> <RIGHT> <OUT>` |

## Code modding

[`code_modding/`](code_modding)

> [!WARNING]
> For code modding MN PC, [hi-octane-hook](https://github.com/itsmeft24/hi-octane-hook) is **archived and deprecated**, and is no longer recommended for use. Treat it as documentation only. If you read it, use the `settings-json` branch, which is the newest. For the non-matching MN PC decompilation, see [high-octane-dev/rre](https://github.com/high-octane-dev/rre).

To unpack scenes and streaming packages, use [offsetting](https://github.com/offsetting/offsetting) or [soiboy](https://github.com/itsmeft24/soiboy); the tools in this repo don't cover them.

| Script | Target | Purpose |
|---|---|---|
| [`pc/hi-octane-hook-patch-exe.py`](code_modding/pc/hi-octane-hook-patch-exe.py) | PC *Mater-National* `cars_mater.exe` | Patches it in place after checking its MD5: large-address aware, SM3 shaders, new turntable rotation, and loading `CarsGDF.dll` in place of `binkw32.dll` so hooks run before `WinMain` |
| [`wii/DIALOGUE_APPEND_MN_WII.py`](code_modding/wii/DIALOGUE_APPEND_MN_WII.py) | Wii *Mater-National* (NTSC-U) | Reads `css_append_mn.txt` (one character per line) and writes `out_mn.asm`, a gctrealmate patch that adds character-select voice clips |
| [`wii/DIALOGUE_APPEND_ROR_WII.py`](code_modding/wii/DIALOGUE_APPEND_ROR_WII.py) | Wii *Race-O-Rama* (NTSC-U) | Same, with `css_append_ror.txt` → `out_ror.asm` |

Run these from the folder that holds the input files.

## Hash labels

[`hash_labels/`](hash_labels) — lists of known file paths used to name hash-only archive entries (see [XBR](file_formats/XBR.md)). **Everything in this folder applies to ROR on Xbox 360 and PS3 only.**

| File | Contents |
|---|---|
| [`xbr_hashes.txt`](hash_labels/xbr_hashes.txt) | Xbox 360 paths (backslashes) |
| [`p3r_hashes.txt`](hash_labels/p3r_hashes.txt) | PS3 paths (forward slashes) |
| [`cracking/interactive.py`](hash_labels/cracking/interactive.py) | Look up a hash or file index in a `cars3.xbr` interactively (`index <n>` or `0x<hash>`) |
| [`cracking/unhashme_gen.py`](hash_labels/cracking/unhashme_gen.py) | Dumps every archive entry missing from `xbr_hashes.txt` into `./unhashme/`, with a file extension guessed from its contents, so you can work out its name (run from `cracking/` with `cars3.xbr` present) |

New label contributions are welcome. The Rust extractor downloads `xbr_hashes.txt` from this repo's `main` branch.

## Repository layout

```
cars-research/
├── code_modding/   exe patches and Wii assembly generators
├── file_formats/   format specifications (Markdown)
├── hash_labels/    known path lists + hash cracking helpers
└── tools/          converters, extractors and Blender add-ons
```

## License

[MIT](LICENSE)
