import bpy
import bmesh
import struct
import mathutils
import os
import math
import configparser

def read_string(f, length):
    raw = f.read(length)
    return raw.split(b'\x00')[0].decode('utf-8', errors='ignore')

def float_array_from_string(_str):
    return [float(component) for component in _str.split(",")]

def apply_static_model_parameters(static_model_armature, pos_data, look_data, up_data):
    # 1. Parse inputs to 3D mathutils Vectors (ignoring the 4th 'W' component)
    pos = mathutils.Vector(pos_data[:3])
    look = mathutils.Vector(look_data[:3])
    up = mathutils.Vector(up_data[:3])
    
    # 2. Swap Y and Z to convert Left-Handed Y-Up to Right-Handed Z-Up
    # Game +X (Right)   -> Blender +X (Right)
    # Game +Z (Forward) -> Blender +Y (Forward)
    # Game +Y (Up)      -> Blender +Z (Up)
    pos_b = mathutils.Vector((pos.x, pos.z, pos.y))
    look_b = mathutils.Vector((look.x, look.z, look.y)).normalized()
    up_b = mathutils.Vector((up.x, up.z, up.y)).normalized()
    
    # 3. Calculate Right vector in Right-Handed space (Forward cross Up)
    right_b = look_b.cross(up_b).normalized()
    
    # Re-orthogonalize the true Up vector
    true_up_b = right_b.cross(look_b).normalized()
    
    # 4. Construct the Rotation Matrix
    # Because we swapped Y and Z above, this matrix will inherently have a determinant of -1.
    # Blender will automatically apply a negative scale to the armature, safely 
    # converting your untouched Left-Handed mesh vertices into Right-Handed space!
    rot_mat = mathutils.Matrix((
        (right_b.x, true_up_b.x, look_b.x),
        (right_b.y, true_up_b.y, look_b.y),
        (right_b.z, true_up_b.z, look_b.z)
    ))
    
    # 5. Apply the coordinates and rotation
    static_model_armature.matrix_basis = rot_mat.to_4x4()
    static_model_armature.location = pos_b


def load_xng(filepath, xng_name):
    with open(filepath, 'rb') as f:
        magic = f.read(4)
        version = struct.unpack('>i', f.read(4))[0]
        numBones = struct.unpack('>i', f.read(4))[0]
        
        bones = []
        if numBones > 0:
            for i in range(numBones):
                name = read_string(f, 128)
                mat_floats = struct.unpack('>16f', f.read(64))
                
                f.seek(12 + 12 + 4, 1)
                
                parent_idx = struct.unpack('>i', f.read(4))[0]

                mat = mathutils.Matrix((
                    mat_floats[0:4],
                    mat_floats[4:8],
                    mat_floats[8:12],
                    mat_floats[12:16]
                ))
                bones.append({'name': name, 'matrix': mat, 'parent': parent_idx})

        numNames = struct.unpack('>i', f.read(4))[0]
        material_names = []
        if numNames > 0:
            for i in range(numNames):
                material_names.append(read_string(f, 64))

        numLOD = struct.unpack('B', f.read(1))[0]
        skinAnimatesFlag = struct.unpack('B', f.read(1))[0]
        unused1 = struct.unpack('B', f.read(1))[0]
        unused2 = struct.unpack('B', f.read(1))[0]

        animates = numBones > 1
        complexMatrixPalettes = numBones > 29 

        saved_offset = f.tell()
        numSurface = 0
        
        for lodIndex in range(numLOD):
            autoLODValue = struct.unpack('>f', f.read(4))[0]
            numMeshes = struct.unpack('>i', f.read(4))[0]
            numSurface += numMeshes

            for meshIndex in range(numMeshes):
                meshID = struct.unpack('>i', f.read(4))[0]
                vertFormat = struct.unpack('>i', f.read(4))[0]

                hasPos = (vertFormat & 1) == 1
                hasNormal = (vertFormat & 2) == 2
                hasUV = (vertFormat & 4) == 4
                hasColor = (vertFormat & 8) == 8
                hasUV2 = (vertFormat & 0x10) == 0x10
                hasTangent = (vertFormat & 0x20) == 0x20
                hasSingleBone = (vertFormat & 0x40) == 0x40
                hasComp = (vertFormat & 0x2000) == 0x2000
                hasSkin = (vertFormat & 0x1000) == 0x1000
                hasUV3 = (vertFormat & 0x4000) == 0x4000
                hasUV4 = (vertFormat & 0x8000) == 0x8000

                if hasComp:
                    f.seek(32, 1) 

                numUV = struct.unpack('B', f.read(1))[0]
                compressed = struct.unpack('B', f.read(1))[0]
                streaming = struct.unpack('B', f.read(1))[0]
                f.seek(3, 1) # Skip 3 unused bytes
                f.seek(numUV * 4, 1)

                numVerts = struct.unpack('>H', f.read(2))[0]
                numFaceIndex = struct.unpack('>H', f.read(2))[0]

                f.seek(numFaceIndex * 2, 1)
                if numFaceIndex % 2 == 1:
                    f.seek(2, 1)

                if hasPos: f.seek(numVerts * 12, 1)
                if hasNormal: f.seek(numVerts * 12, 1)
                if hasColor: f.seek(numVerts * 4, 1)
                if hasUV: f.seek(numVerts * 8, 1)
                if hasSingleBone: f.seek(numVerts * 4, 1)
                if hasSkin: f.seek(numVerts * 32, 1)
                if hasUV2: f.seek(numVerts * 8, 1)
                if hasUV3: f.seek(numVerts * 8, 1)
                if hasUV4: f.seek(numVerts * 8, 1)
                if hasTangent: f.seek(numVerts * 12, 1)

        boneMaps = []
        boneMapIndices = []
        
        if animates:
            info = struct.unpack('>i', f.read(4))[0]
            numBoneMaps = info & 0xffff
            if numBoneMaps > 0:
                for _ in range(numBoneMaps):
                    numBoneIDs = struct.unpack('>i', f.read(4))[0]
                    bmap = [struct.unpack('B', f.read(1))[0] for _ in range(numBoneIDs)]
                    boneMaps.append(bmap)

                if numSurface > 1 and complexMatrixPalettes:
                    for _ in range(numSurface):
                        boneMapIndices.append(struct.unpack('B', f.read(1))[0])
                else:
                    boneMapIndices.append(0)

        f.seek(saved_offset)

        arm_obj = None
        if numBones > 0:
            arm_data = bpy.data.armatures.new(xng_name)
            arm_obj = bpy.data.objects.new(xng_name, arm_data)
            bpy.context.collection.objects.link(arm_obj)
            bpy.context.view_layer.objects.active = arm_obj
            bpy.ops.object.mode_set(mode='EDIT')

            edit_bones = []
            for b_info in bones:
                eb = arm_data.edit_bones.new(b_info['name'])
                eb.tail = (0, 0.1, 0)
                eb.matrix = b_info['matrix']
                edit_bones.append(eb)

            for i, b_info in enumerate(bones):
                if b_info['parent'] >= 0:
                    edit_bones[i].parent = edit_bones[b_info['parent']]

            bpy.ops.object.mode_set(mode='OBJECT')

        b_mats = []
        for m_name in material_names:
            mat = bpy.data.materials.get(m_name)
            if not mat:
                mat = bpy.data.materials.new(m_name)
            b_mats.append(mat)

        # Process LOD0 only
        if numLOD > 0:
            autoLODValue = struct.unpack('>f', f.read(4))[0]
            numMeshes = struct.unpack('>i', f.read(4))[0]

            for meshIndex in range(numMeshes):
                meshID = struct.unpack('>i', f.read(4))[0]
                vertFormat = struct.unpack('>i', f.read(4))[0]

                hasPos = (vertFormat & 1) == 1
                hasNormal = (vertFormat & 2) == 2
                hasUV = (vertFormat & 4) == 4
                hasColor = (vertFormat & 8) == 8
                hasUV2 = (vertFormat & 0x10) == 0x10
                hasTangent = (vertFormat & 0x20) == 0x20
                hasSingleBone = (vertFormat & 0x40) == 0x40
                hasComp = (vertFormat & 0x2000) == 0x2000
                hasSkin = (vertFormat & 0x1000) == 0x1000
                hasUV3 = (vertFormat & 0x4000) == 0x4000
                hasUV4 = (vertFormat & 0x8000) == 0x8000

                if hasComp: f.seek(32, 1)

                numUV = struct.unpack('B', f.read(1))[0]
                f.seek(5, 1)
                f.seek(numUV * 4, 1)

                numVerts = struct.unpack('>H', f.read(2))[0]
                numFaceIndex = struct.unpack('>H', f.read(2))[0]

                indices = struct.unpack(f'>{numFaceIndex}H', f.read(numFaceIndex * 2))

                boneMap = []
                if animates:
                    bm_idx = meshID if complexMatrixPalettes else 0
                    if bm_idx < len(boneMapIndices) and boneMapIndices[bm_idx] < len(boneMaps):
                        boneMap = boneMaps[boneMapIndices[bm_idx]]

                if numFaceIndex % 2 == 1: f.seek(2, 1)

                verts = []
                if hasPos:
                    for _ in range(numVerts):
                        verts.append(struct.unpack('>3f', f.read(12)))

                normals = []
                if hasNormal:
                    for _ in range(numVerts):
                        normals.append(struct.unpack('>3f', f.read(12)))

                colors = []
                if hasColor:
                    for _ in range(numVerts):
                        a, r, g, b = struct.unpack('>4B', f.read(4))
                        colors.append((r / 255.0, g / 255.0, b / 255.0, a / 255.0))

                uvs = []
                if hasUV:
                    for _ in range(numVerts):
                        uv = struct.unpack('>2f', f.read(8))
                        uvs.append((uv[0], 1.0 - uv[1]))

                v_bone_ids = [[] for _ in range(numVerts)]
                v_bone_wgts = [[] for _ in range(numVerts)]

                if hasSingleBone and not hasSkin:
                    for v in range(numVerts):
                        val = struct.unpack('>f', f.read(4))[0]
                        idx = boneMap[int(val)] if (animates and int(val) < len(boneMap)) else int(val)
                        v_bone_ids[v].append(idx)
                        v_bone_wgts[v].append(1.0)
                elif hasSingleBone:
                    f.seek(numVerts * 4, 1)

                if hasSkin:
                    for v in range(numVerts):
                        ids = struct.unpack('>4f', f.read(16))
                        for bid in ids:
                            idx = boneMap[int(bid)] if (animates and int(bid) < len(boneMap)) else int(bid)
                            v_bone_ids[v].append(idx)

                    for v in range(numVerts):
                        wgts = struct.unpack('>4f', f.read(16))
                        v_bone_wgts[v].extend(wgts)

                uvs2 = []
                if hasUV2:
                    for _ in range(numVerts):
                        uv = struct.unpack('>2f', f.read(8))
                        uvs2.append((uv[0], 1.0 - uv[1]))
                        
                uvs3 = []
                if hasUV3:
                    for _ in range(numVerts):
                        uv = struct.unpack('>2f', f.read(8))
                        uvs3.append((uv[0], 1.0 - uv[1]))
                        
                uvs4 = []
                if hasUV4:
                    for _ in range(numVerts):
                        uv = struct.unpack('>2f', f.read(8))
                        uvs4.append((uv[0], 1.0 - uv[1]))

                if hasTangent: f.seek(numVerts * 12, 1)

                mesh_name = f"{material_names[meshID]}" if material_names else f"Mesh_{meshIndex}"
                me = bpy.data.meshes.new(mesh_name)
                ob = bpy.data.objects.new(mesh_name, me)
                bpy.context.collection.objects.link(ob)

                if material_names and meshID < len(b_mats):
                    me.materials.append(b_mats[meshID])

                if arm_obj:
                    mod = ob.modifiers.new("Armature", 'ARMATURE')
                    mod.object = arm_obj
                    ob.parent = arm_obj

                tris = []
                for i in range(len(indices) - 2):
                    v0, v1, v2 = indices[i], indices[i+1], indices[i+2]
                    if v0 == v1 or v1 == v2 or v2 == v0:
                        continue
                    if i % 2 == 1:
                        tris.append((v1, v0, v2))
                    else:
                        tris.append((v0, v1, v2))

                me.from_pydata(verts, [], tris)
                me.update()

                if hasNormal and normals:
                    me.normals_split_custom_set_from_vertices(normals)

                if hasColor and colors:
                    color_layer = me.color_attributes.new(name="Color", type='FLOAT_COLOR', domain='POINT')
                    for v in me.vertices:
                        color_layer.data[v.index].color = colors[v.index]

                if hasUV and uvs:
                    uv_layer = me.uv_layers.new(name="UV0")
                    for loop in me.loops:
                        uv_layer.data[loop.index].uv = uvs[loop.vertex_index]
                if hasUV2 and uvs2:
                    uv_layer = me.uv_layers.new(name="UV1")
                    for loop in me.loops:
                        uv_layer.data[loop.index].uv = uvs2[loop.vertex_index]
                if hasUV3 and uvs3:
                    uv_layer = me.uv_layers.new(name="UV2")
                    for loop in me.loops:
                        uv_layer.data[loop.index].uv = uvs3[loop.vertex_index]
                if hasUV4 and uvs4:
                    uv_layer = me.uv_layers.new(name="UV3")
                    for loop in me.loops:
                        uv_layer.data[loop.index].uv = uvs4[loop.vertex_index]

                if arm_obj and (hasSingleBone or hasSkin):
                    groups = {}
                    for i, b_info in enumerate(bones):
                        groups[i] = ob.vertex_groups.new(name=b_info['name'])

                    for v_idx in range(numVerts):
                        for bid, bwgt in zip(v_bone_ids[v_idx], v_bone_wgts[v_idx]):
                            if bwgt > 0.0 and bid in groups:
                                groups[bid].add([v_idx], bwgt, 'REPLACE')

def load_scn(scn_file_path, folder):
    tom = configparser.ConfigParser()
    tom.read(scn_file_path)
    count = int(tom["StaticModels"]["NumberOfStaticModels"])
    for i in range(count):
        slt_name = tom[f"Model{i + 1}"]["SLT"]
        position = float_array_from_string(tom[f"Model{i + 1}"]["Position"])
        look_vector = [0,0,1,1] if "LookVector" not in tom[f"Model{i + 1}"] else float_array_from_string(tom[f"Model{i + 1}"]["LookVector"])
        up_vector = [0,1,0,1] if "UpVector" not in tom[f"Model{i + 1}"] else float_array_from_string(tom[f"Model{i + 1}"]["UpVector"])
        
        load_xng(f"{folder}\\Geo\\{slt_name}.xng", f"StaticModel{i + 1}_{slt_name}")
        apply_static_model_parameters(bpy.data.objects[f"StaticModel{i + 1}_{slt_name}"], position, look_vector, up_vector)

# load_xng(r"C:\Projects\RRE\Cars2\PC\Development-Upstream\Hi-Octane\C\Global\Chars\Emma\Anim\Emma_Reg.xng", "Emma_Reg")
load_scn(r"C:\Projects\RRE\Cars2\PC\Development-Upstream\Hi-Octane\C\Scenes\FE\FE.x360.scn", r"C:\Projects\RRE\Cars2\PC\Development-Upstream\Hi-Octane\C\Scenes\FE")