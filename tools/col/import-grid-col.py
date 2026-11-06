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

def add_mesh(name, verts, faces, col_name="Collection"):    
    mesh = bpy.data.meshes.new(name)
    obj = bpy.data.objects.new(mesh.name, mesh)
    col = bpy.data.collections.get(col_name)
    col.objects.link(obj)
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)

# parse TerrainFile struct
file = open("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Cars Mater-National\\Hi-Octane\\C\\Scenes\\RS\\Col\\RS_terrain.grid.col", "rb")

file_version_id, version, flags_id, flags, file_extension_offset_id, file_extension_offset, material_depth_id, max_depth, max_height, record_index_data_id, number_of_terrain_data_records = struct.unpack("<bfbibibffbi", file.read(29))

terrain_data_record_offset_index_entries = []

for i in range(number_of_terrain_data_records):
    terrain_data_record_offset_index_entries.append(TerrainDataRecordOffsetIndexEntry.from_le_bytes(file.read(20)))

for i in range(number_of_terrain_data_records):
    terrain_data_record_id, faces_id, number_of_faces = struct.unpack("<bbi", file.read(6))
    
    index_buffer = []
    for j in range(number_of_faces):
        index_buffer.append(struct.unpack("<iii", file.read(12)))
    
    vertex_id, number_of_vertices = struct.unpack("<bi", file.read(5))
    vertex_buffer = []
    for j in range(number_of_vertices):
        vertex_buffer.append(struct.unpack("<fff", file.read(12)))
    
    channel_data_id, channel_data_bitfield = struct.unpack("<bb", file.read(2))
    if (channel_data_bitfield & 1) == 1:
        data_1 = file.read(number_of_vertices)
        unique_data_count_1 = int.from_bytes(file.read(4), 'little')
        unique_data_1 = file.read(unique_data_count_1)
    if (channel_data_bitfield & 2) == 2:
        data_2 = file.read(number_of_vertices)
        unique_data_count_2 = int.from_bytes(file.read(4), 'little')
        unique_data_2 = file.read(unique_data_count_2)
    if (channel_data_bitfield & 4) == 4:
        data_3 = file.read(number_of_vertices)
        unique_data_count_3 = int.from_bytes(file.read(4), 'little')
        unique_data_3 = file.read(unique_data_count_3)
    if (channel_data_bitfield & 8) == 8:
        data_4 = file.read(number_of_vertices)
        unique_data_count_4 = int.from_bytes(file.read(4), 'little')
        unique_data_4 = file.read(unique_data_count_4)
    if (channel_data_bitfield & 16) == 16:
        data_5 = file.read(number_of_vertices)
        unique_data_count_5 = int.from_bytes(file.read(4), 'little')
        unique_data_5 = file.read(unique_data_count_5)
    if (channel_data_bitfield & 32) == 32:
        data_6 = file.read(number_of_vertices)
        unique_data_count_6 = int.from_bytes(file.read(4), 'little')
        unique_data_6 = file.read(unique_data_count_6)
    if (channel_data_bitfield & 64) == 64:
        data_7 = file.read(number_of_vertices)
        unique_data_count_7 = int.from_bytes(file.read(4), 'little')
        unique_data_7 = file.read(unique_data_count_7)
    if (channel_data_bitfield & 128) == 128:
        data_8 = file.read(number_of_vertices)
        unique_data_count_8 = int.from_bytes(file.read(4), 'little')
        unique_data_8 = file.read(unique_data_count_8)
    
    add_mesh("mesh_"+str(i), vertex_buffer, index_buffer)