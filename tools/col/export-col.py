import struct, bpy

class TerrainDataRecordOffsetIndexEntry:
    def __init__(self, file_offset, min_range, max_range):
        self.file_offset = file_offset
        self.min_range = [min_range[0], min_range[1]]
        self.max_range = [max_range[0], max_range[1]]
    def to_le_bytes(self):
        return struct.pack("<iffff", self.file_offset, self.min_range[0], self.min_range[1], self.max_range[0], self.max_range[1])
    def from_le_bytes(bytes):
        file_offset, min_range_x, min_range_y, max_range_x, max_range_y = struct.unpack("<iffff", bytes)
        return TerrainDataRecordOffsetIndexEntry(file_offset, (min_range_x, min_range_y), (max_range_x, max_range_y))

class TreeNodeValue:
    def __init__(self, dimension, _min, _max):
        self.dimension = dimension
        self._min = _min
        self._max = _max
    def to_le_bytes(self):
        return struct.pack("<bff", self.dimension, self._min, self._max)
    def from_le_bytes(bytes):
        return TreeNodeValue(*struct.unpack("<bff", bytes))

out = open("D:\\SteamLibrary\\steamapps\\common\\Cars Mater-National\\Mods\\MARIO_CIRCUIT\\C\\Scenes\\MK_MC\\Col\\MK_MC.grid.col", "wb")

types = [7,7,7,7,7]
records = ["polygon0", "polygon1", "polygon37", "polygon40", "polygon41"]

file_extension_offset = 0x5000
out.write(struct.pack("<bfbibibffbi", 0, 2.1, 1, 0, 2, file_extension_offset, 3, 0.0, 0.0, 4, len(records)))

offset = 0x1D + len(records) * 20
for i in range(len(records)):
    out.write(TerrainDataRecordOffsetIndexEntry(offset, (0, 1), (0, 1)).to_le_bytes())
    
    mesh = bpy.context.scene.objects[records[i]].data
    offset += len(mesh.polygons) * 12 + len(mesh.vertices) * 12 + 13

pos = out.tell()
out.seek(0xB)
out.write(offset.to_bytes(4, 'little'))
out.seek(pos)

for name in records:
    mesh = bpy.context.scene.objects[name].data
    
    out.write(struct.pack("<bbi", 5, 6, len(mesh.polygons)))
    for face in mesh.polygons:
        out.write(struct.pack("<iii", face.vertices[0], face.vertices[1], face.vertices[2]))
    
    out.write(struct.pack("<bi", 7, len(mesh.vertices)))
    for vertex in mesh.vertices:
        out.write(struct.pack("<fff", vertex.co[0], vertex.co[1], vertex.co[2]))
    
    out.write(struct.pack("<bb", 8, 0))
out.write(b"This is padding for gamecube. \x00")
out.close()