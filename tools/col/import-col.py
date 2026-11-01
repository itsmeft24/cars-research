import bpy, enum, math, struct
from mathutils import *

def add_mesh(name, verts, faces, normals=None, colors=None, edges=None, col_name="Collection"):    
    if edges is None:
        edges = []
    mesh = bpy.data.meshes.new(name)
    obj = bpy.data.objects.new(mesh.name, mesh)
    col = bpy.data.collections.get(col_name)
    col.objects.link(obj)
    mesh.from_pydata(verts, edges, faces)
    if edges is None:
        mesh.update(calc_edges=True)
    if normals is not None:
        if len(normals) != 0:
            mesh.normals_split_custom_set_from_vertices(normals)

f = open("D:\\Xbox 360\\RoRX360_Extracted\\RELEASE (NEW)\\C\\Scenes\\CR_03\\Col\\COLLWALL1.gol", "rb")
# f = open("D:\\GigaLeak\\Rs_A_Mn\\Out\\C\\Scenes\\RS_A\\Col\\World_Collision_B.gol", "rb")
# f = open("D:\\SteamLibrary\\steamapps\\common\\Cars Mater-National\\Hi-Octane\\C\\Global\\Chars\\McQ\\Col\\McQF.gol", "rb")

def get_leaf_index1(tree_face_index, tree_faces):
    print(tree_face_index)
    branch_1 = tree_faces[tree_face_index][7]
    if tree_faces[branch_1][7] & 0x8000 == 0x8000:
        return tree_faces[branch_1][7] - 0x8000
    return get_leaf_index1(branch_1, tree_faces)
    
def get_leaf_index2(tree_face_index, tree_faces):
    branch_2 = tree_faces[tree_face_index][8]
    if tree_faces[branch_2][7] & 0x8000 == 0x8000:
        return tree_faces[branch_2][7] - 0x8000
    return get_leaf_index2(branch_2, tree_faces)

header = struct.unpack(">i4sii", f.read(16))
assert header[0] == 1234
assert header[1] == b"col\x00"
assert header[2] == 2

# Soultree
if header[3] == 0:
    obj_header = struct.unpack(">i12fiiiii", f.read(72))
    
    temp_cmt = obj_header[0]
    obbdata = obj_header[1]
    reverse_collision_mode = obj_header[13]
    vertex_count = obj_header[14]
    tree_face_count = obj_header[15]
    tree_face_leaf_count = obj_header[16]
    quantized = obj_header[17]
    
    vertices = []
    faces = []
    
    if quantized == 1:
        for x in range(vertex_count):
            v = struct.unpack(">hhh", f.read(6))
            vertices.append((v[0] / 32767.0, v[2] / 32767.0, v[1] / 32767.0))
    else:
        for x in range(vertex_count):
            v = struct.unpack(">fff", f.read(12))
            vertices.append((v[0], v[2], v[1]))
    
    load_normals = struct.unpack(">i", f.read(4))[0]
    normals = None
    
    if load_normals:
        normals = []
        if quantized == 1:
            for x in range(vertex_count):
                v = struct.unpack(">hhh", f.read(6))
                normals.append((v[0] / 32767.0, v[2] / 32767.0, v[1] / 32767.0))
        else:
            for x in range(vertex_count):
                v = struct.unpack(">fff", f.read(12))
                normals.append((v[0], v[2], v[1]))
    tree_faces = []
    for x in range(tree_face_count):
        tree_face = struct.unpack(">f3f3f2H", f.read(32))
        tree_faces.append(tree_face)
    for x in range(tree_face_leaf_count):
        tree_face_leaf = struct.unpack(">f3f3h", f.read(22))
        indices = (tree_face_leaf[4],tree_face_leaf[5],tree_face_leaf[6])
        faces.append(indices)
    # add_mesh("mario", vertices, faces, normals)
    obj = bpy.context.scene.objects["mario"]
    # bpy.context.view_layer.objects.active = obj
    # bpy.ops.object.mode_set(mode='EDIT')
    obj.data.polygons[get_leaf_index1(0, tree_faces)].select = True
    obj.data.polygons[get_leaf_index2(0, tree_faces)].select = True