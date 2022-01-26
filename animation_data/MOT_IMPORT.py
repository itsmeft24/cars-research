#!BPY
import bpy, struct
from mathutils import *
from MOTLib import *

bl_info = {
    "name": "Cars: Mater-National Animation Importer",
    "description": "Imports animation data from GOT/WOT files (binary animation format used by some games developed by Rainbow Studios and Incinerator Studios) onto a selected armature.",
    "author": "itsmeft24, Carlos",
    "version": (1, 0, 0),
    "blender": (2, 90, 0),
    "location": "File > Import",
    "category": "Import-Export"
}

def insert_byte_angle_axis_key(arm_name, bone_name, frame_idx, key):
    if bone_name == arm_name:
        armature = bpy.data.objects[arm_name]
        armature.rotation_mode = 'AXIS_ANGLE'
        armature.rotation_axis_angle[0] = key.Angle/BYTE_ANGLE_AXIS_KEY_SCALE
        armature.rotation_axis_angle[1] = normalize_from_i8_to_1f(key.AxisX)
        armature.rotation_axis_angle[2] = normalize_from_i8_to_1f(key.AxisY)
        armature.rotation_axis_angle[3] = normalize_from_i8_to_1f(key.AxisZ)
        armature.keyframe_insert(data_path="rotation_axis_angle", frame=frame_idx)
        return
    armature = bpy.data.objects[arm_name]
    bpy.ops.object.mode_set(mode='POSE')

    pbone = armature.pose.bones[bone_name]
    pbone.rotation_mode = 'QUATERNION'
    
    axis = Vector((normalize_from_i8_to_1f(key.AxisX), normalize_from_i8_to_1f(key.AxisY), normalize_from_i8_to_1f(key.AxisZ)))
    
    angle = key.Angle/BYTE_ANGLE_AXIS_KEY_SCALE
    
    quat = Quaternion(axis, angle)
    
    parent_matrix = pbone.parent.matrix if pbone.parent else Matrix()
    parent_mat_inverted = parent_matrix.inverted()
    relative_mat = parent_mat_inverted @ pbone.matrix
    trans = relative_mat.to_translation()
    
    pbone.matrix = parent_matrix @ Matrix.Translation(trans) @ quat.to_matrix().to_4x4()
    
    bpy.ops.object.mode_set(mode='OBJECT')
    pbone.keyframe_insert(data_path="rotation_quaternion", frame=frame_idx)

def insert_short_angle_axis_key(arm_name, bone_name, frame_idx, key):
    if bone_name == arm_name:
        armature = bpy.data.objects[arm_name]
        armature.rotation_mode = 'AXIS_ANGLE'
        armature.rotation_axis_angle[0] = key.Angle/SHORT_ANGLE_AXIS_KEY_SCALE
        armature.rotation_axis_angle[1] = normalize_from_i16_to_1f(key.AxisX)
        armature.rotation_axis_angle[2] = normalize_from_i16_to_1f(key.AxisY)
        armature.rotation_axis_angle[3] = normalize_from_i16_to_1f(key.AxisZ)
        armature.keyframe_insert(data_path="rotation_axis_angle", frame=frame_idx)
        return
    armature = bpy.data.objects[arm_name]
    bpy.ops.object.mode_set(mode='POSE')

    pbone = armature.pose.bones[bone_name]
    pbone.rotation_mode = 'QUATERNION'
    
    axis = Vector((normalize_from_i16_to_1f(key.AxisX), normalize_from_i16_to_1f(key.AxisY), normalize_from_i16_to_1f(key.AxisZ)))
    
    angle = key.Angle/SHORT_ANGLE_AXIS_KEY_SCALE
    
    quat = Quaternion(axis, angle)
    
    parent_matrix = pbone.parent.matrix if pbone.parent else Matrix()
    parent_mat_inverted = parent_matrix.inverted()
    relative_mat = parent_mat_inverted @ pbone.matrix
    trans = relative_mat.to_translation()
    
    pbone.matrix = parent_matrix @ Matrix.Translation(trans) @ quat.to_matrix().to_4x4()
    
    bpy.ops.object.mode_set(mode='OBJECT')
    pbone.keyframe_insert(data_path="rotation_quaternion", frame=frame_idx)

def insert_quaternion_key(arm_name, bone_name, frame_idx, key):
    if bone_name == arm_name:
        armature = bpy.data.objects[arm_name]
        armature.rotation_mode = 'QUATERNION'
        armature.rotation_quaternion = Quaternion((key.W, key.X, key.Y, key.Z))
        armature.keyframe_insert(data_path="rotation_quaternion", frame=frame_idx)
        return
    armature = bpy.data.objects[arm_name]
    bpy.ops.object.mode_set(mode='POSE')

    pbone = armature.pose.bones[bone_name]
    pbone.rotation_mode = 'QUATERNION'
    
    quat = Quaternion((key.W, key.X, key.Y, key.Z))
    
    parent_matrix = pbone.parent.matrix if pbone.parent else Matrix()
    parent_mat_inverted = parent_matrix.inverted()
    relative_mat = parent_mat_inverted @ pbone.matrix
    trans = relative_mat.to_translation()
    
    pbone.matrix = parent_matrix @ Matrix.Translation(trans) @ quat.to_matrix().to_4x4()
    
    bpy.ops.object.mode_set(mode='OBJECT')
    pbone.keyframe_insert(data_path="rotation_quaternion", frame=frame_idx)
    
def insert_position_key(arm_name, bone_name, frame_idx, x, y, z):
    if bone_name == arm_name:
        armature = bpy.data.objects[arm_name]
        armature.location = (x, y, z)
        armature.keyframe_insert(data_path="location", frame=frame_idx)
        return
    
    armature = bpy.data.objects[arm_name]
    bpy.ops.object.mode_set(mode='POSE')

    pbone = armature.pose.bones[bone_name]
    
    parent_matrix = pbone.parent.matrix if pbone.parent else Matrix()
    
    pbone.matrix = parent_matrix @ Matrix.Translation([x, y, z])
    
    bpy.ops.object.mode_set(mode='OBJECT')
    pbone.keyframe_insert(data_path="location", frame=frame_idx)

from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

class ImportSomeData(Operator, ImportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "import_anim.got"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Import .GOT/WOT"

    # ExportHelper mixin class uses this
    filename_ext = ".got"

    filter_glob: StringProperty(
        default="*.got",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )
    
    def execute(self, context):
        return import_mot_main(context, self.filepath)

def import_mot_main(ctx, path):
    anim = open(path, "rb")
    
    if ctx.selected_objects[0].type == 'ARMATURE':
        arm_name = ctx.selected_objects[0].name
    
    hdr = struct.unpack(">iIII", anim.read(16))
    version = hdr[0]
    mot_type = hdr[1]
    framecount = hdr[2]
    object_count = hdr[3]
    
    objects = []
    for x in range(object_count):
        len = struct.unpack(">I", anim.read(4))[0]
        objects.append(anim.read(len)[:-1].decode())
    
    hdr_after_table = struct.unpack(">fIIIII", anim.read(24))
    
    duration = hdr_after_table[0]
    rot_type = hdr_after_table[1]
    pos_type = hdr_after_table[2]
    num_pos_key = hdr_after_table[3]
    num_rot_key = hdr_after_table[4]
    num_fov_key = hdr_after_table[5]
    
    print("Version: "+str(version))
    
    if mot_type == 0:
        print("Camera animations are not supported. Please try again with a standard MOT/WOT/GOT animation.")
        return {'CANCELLED'}
    
    if pos_type == 1:
        print("BezierPosition")
    elif pos_type == 0:
        print("LinearPosition")
    
    if rot_type == 3:
        print("HalfQuaternion")
    if rot_type == 2:
        print("Quaternion")
    elif rot_type == 1:
        print("ShortAxis")
    elif rot_type == 0:
        print("ByteAxis")
    
    anim.seek(4, 1)
    
    for x in range(num_pos_key):
        POSTABLE = struct.unpack(">II", anim.read(8))
        number_of_frames_in_key = POSTABLE[0]
        object_index = POSTABLE[1]
        
        for x in range(number_of_frames_in_key):
            
            if pos_type == 1:
                key = BezierPositionKey(anim.read(40))
            elif pos_type == 0:
                key = LinearPositionKey(anim.read(16))
            insert_position_key(arm_name, objects[object_index], ms_to_frame(key.Time), key.PosX, key.PosY, key.PosZ)
    
    for x in range(num_rot_key):
        ROTTABLE = struct.unpack(">II", anim.read(8))
        number_of_frames_in_key = ROTTABLE[0]
        object_index = ROTTABLE[1]
        
        for x in range(number_of_frames_in_key):
            
            if rot_type == 3:
                key = HalfQuaternionRotationKey(anim.read(12))
                insert_quaternion_key(arm_name, objects[object_index], ms_to_frame(key.Time), key)
            elif rot_type == 2:
                key = QuaternionRotationKey(anim.read(20))
                insert_quaternion_key(arm_name, objects[object_index], ms_to_frame(key.Time), key)
            elif rot_type == 1:
                key = ShortRotationKey(anim.read(12))
                insert_short_angle_axis_key(arm_name, objects[object_index], ms_to_frame(key.Time), key)
            elif rot_type == 0:
                key = ByteRotationKey(anim.read(8))
                insert_byte_angle_axis_key(arm_name, objects[object_index], ms_to_frame(key.Time), key)
    
    return {"FINISHED"}

def menu_func_import(self, context):
    self.layout.operator(ImportSomeData.bl_idname, text="GOT/WOT (.got)")

def register():
    bpy.utils.register_class(ImportSomeData)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)

def unregister():
    bpy.utils.unregister_class(ImportSomeData)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)

if __name__ == "__main__":
    register()
