# BIN + LOC — Localization

Localized text is split across a pair of files: a `.loc` file holding the text IDs and size limits, and a `.bin` file holding the strings.

| | |
|---|---|
| Byte order | Little-endian |
| Tools | [`tools/bin-loc/bin_loc_json.py`](../tools/bin-loc/bin_loc_json.py) (to JSON), [`json_to_cars.py`](../tools/bin-loc/json_to_cars.py) (back to BIN/LOC) |

## `.loc`

| Type | Description |
|---|---|
| u32 | Number of strings |
| u32 | Size of the ID string block |
| char[] | Null-terminated text IDs, one per string |
| u16[n] | Max size of each string |

## `.bin`

| Type | Description |
|---|---|
| u32 | Size of the string block + 2 |
| u16 | UTF-16 BOM (`0xFEFF`) |
| UTF-16LE[] | Strings, each terminated by `00 00` |
| per-entry records | One record per `.loc` entry, below |

Per-entry record:

| Type | Description |
|---|---|
| u8 | `IsDynamic` |
| u8 | Number of strings this entry appends (`n`) |
| u32[n] | Indices of the appended strings — only if `n > 0` |
| u16[n] | Positions of each insertion — only if `n > 0` |
| u8 | Number of entries that append to this one (`m`) |
| u32[m] | Indices of those entries — only if `m > 0` |

When `n > 0`, an extra copy of the entry's string is stored right after it in the string block (`CopiedToEnd` in the JSON). Strings made only of `X` characters are written as UTF-8 by `json_to_cars.py -utf8XXX`, because the game uses them as placeholders.

## JSON schema

```json
{
  "TextID": "...", "Value": "...", "MaxSize": 64, "IsDynamic": 0,
  "Appends": [], "AtPositions": [], "CopiedToEnd": "...", "IsAppendedTo": []
}
```

The last four keys are optional.

## Notes

- `bin_loc_json.py` takes the `.loc` first, then the `.bin` (its usage text now says so).
- The meaning of the append fields is inferred from the code, which uses Ghidra-style variable names (`local_125`, `pbVar12`).
