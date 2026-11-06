# SCN — Scene Description

`.scn` (e.g. `FE.x360.scn`) is a plain-text INI file that places static models in a scene. It is read with Python's `configparser` in [`tools/scn/import-scn.py`](../tools/scn/import-scn.py).

## Sections used

```ini
[StaticModels]
NumberOfStaticModels = 2

[Model1]
SLT = SomeModelName          ; loads Geo\SomeModelName.xng
Position = x,y,z,w
LookVector = 0,0,1,1         ; optional, default shown
UpVector = 0,1,0,1           ; optional, default shown
```

- Each `[ModelN]` (1-based) references an [XNG](XNG_DXG.md) file at `<scene folder>\Geo\<SLT>.xng`.
- Vectors are four comma-separated floats; the importer ignores the fourth (W).
- The game's coordinate system is left-handed and Y-up. The importer maps game (X, Y, Z) to Blender (X, Z, Y).

## Not documented

Only the keys read by the importer are listed here. Real scene files contain many more sections (lighting, triggers, cameras, and so on) that haven't been catalogued.
