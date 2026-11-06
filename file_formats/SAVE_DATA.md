# Save Data (`PSLOT*.dat`)

Mater-National **PC** profile save files (for example `Documents\THQ\Cars2\PSLOT0.dat`). Documented by the `Curio` parser in [`tools/save-data`](../tools/save-data), which is specific to MN PC. It is a work in progress that only reads the file and prints the end offset.

| | |
|---|---|
| Byte order | Little-endian, packed structures |
| Needs | `DataPC\C\Act\EventInfo.dat`, `Global\Chars\VehicleInfo.dat`, `Story\StoryInfo.dat` and `Act\ScavengerHuntData.txt` from the game, because several arrays have game-defined lengths |

## Sections (in file order)

| # | Section | Contents |
|---|---|---|
| 1 | Profile (0x2D bytes) | `char Name[20]`, bolt banners won, points, percent complete, starting banners, starting bonus points |
| 2 | Settings | Basic settings (0x1E bytes: difficulty, audio on/off, music/effects/dialogue volume, map/stats layer, km/h vs mph, anti-aliasing, …); three playlists (`u8 count` + song ids); extra settings (7 bytes: time of day, auto-recentering, big air, TT cam, resolution) |
| 3 | Event database | For each event set in `EventInfo.dat`: for each event, `{i32 Status, i32 Flags}` |
| 4 | Vehicle database | For each vehicle in `VehicleInfo.dat`: paint-job lock records (`u32`) and wheel sets (5 × `u32`) |
| 5 | Story manager | Story events (`u32 status`, `u32[3]` bolt banners earned), current vehicle index, normal and large vehicle info (3 × `char[40]` each), 7 bumper stickers, 4 explore hubs (2 × `char[40]`), current hub index, time of day, current story event name (`char[40]`), 2 unknowns |
| 6 | Scavenger hunt | Metal detector flag, part-group index, then per group: locked, unknown, `u32` flag per part, unknown; trailing unknown |
| 7 | Personal records | `u32` timestamp, profile names (`char[32]`), 59 record sets (`char[64]` name, count, records of `{type, 3 × (profile index, f32 value)}`) |
| 8 | Bonus content | 89 `u32` unlock flags |

Record types: 1 fastest time, 2 slowest time, 3 fastest race, 4 most laps, 5 highest score, 6 most laughs, 7 best single lap.

## Not yet parsed

Pickup manager and bolt manager (commented out in the code).

## Notes

- `Curio` takes `<DATAPC_DIR>` and `<SAVE_FILE>` as CLI arguments.
- It includes static assertions on structure sizes (`sizeof(CarsProfile) == 0x2D`, `sizeof(CarsBasicSettings) == 0x1E`).
- Many fields are still `Unk*`.
