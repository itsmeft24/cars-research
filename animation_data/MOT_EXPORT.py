#!BPY
import bpy, struct
from mathutils import *
from MOTLib import *

bl_info = {
    "name": "Cars: Mater-National Animation Exporter",
    "description": "Exports animation data to GOT/WOT files (binary animation format used by some games developed by Rainbow Studios and Incinerator Studios)",
    "author": "itsmeft24, Carlos",
    "version": (1, 0, 0),
    "blender": (2, 90, 0),
    "location": "File > Export",
    "category": "Import-Export"
}

def get_position_key(ctx, armature, bone_name, frame_idx, framerate):
    
    if armature.name == bone_name:
        
        ctx.scene.frame_set(frame_idx)
        
        trans = armature.location
        
        lpk = LinearPositionKey.new(frame_to_ms(frame_idx, framerate), trans.x, trans.y, trans.z)
        
        return lpk
    
    ctx.scene.frame_set(frame_idx)
    
    pbone = armature.pose.bones[bone_name]
    
    parent_mat_inverted = pbone.parent.matrix.inverted() if pbone.parent else Matrix()
    relative_mat = parent_mat_inverted @ pbone.matrix
    trans = relative_mat.to_translation()
    
    lpk = LinearPositionKey.new(frame_to_ms(frame_idx, framerate), trans.x, trans.y, trans.z)
    
    return lpk

def get_rotation_key(ctx, armature, bone_name, frame_idx, framerate):

    if armature.name == bone_name:
        
        ctx.scene.frame_set(frame_idx)
        
        angle = round(armature.rotation_axis_angle[0]*SHORT_ANGLE_AXIS_KEY_SCALE)
        
        srk = ShortRotationKey.new(
            frame_to_ms(frame_idx, framerate),
            angle,
            denormalize_from_1f_to_i16(armature.rotation_axis_angle[1]),
            denormalize_from_1f_to_i16(armature.rotation_axis_angle[2]),
            denormalize_from_1f_to_i16(armature.rotation_axis_angle[3])
        )
        
        return srk
        
    ctx.scene.frame_set(frame_idx)
    
    pbone = armature.pose.bones[bone_name]
    
    parent_mat_inverted = pbone.parent.matrix.inverted() if pbone.parent else Matrix()
    relative_mat = parent_mat_inverted @ pbone.matrix
    rot = relative_mat.to_quaternion()
    
    axis, angle = rot.to_axis_angle()
    
    axis = axis.normalized()
    
    srk = ShortRotationKey.new(
        frame_to_ms(frame_idx, framerate),
        round(angle*SHORT_ANGLE_AXIS_KEY_SCALE),
        denormalize_from_1f_to_i16(axis.x),
        denormalize_from_1f_to_i16(axis.y),
        denormalize_from_1f_to_i16(axis.z)
    )
    
    return srk

from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

class ExportSomeData(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_anim.got"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export .GOT/WOT"

    # ExportHelper mixin class uses this
    filename_ext = ".got"

    filter_glob: StringProperty(
        default="*.got",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )
    
    def execute(self, context):
        return export_mot_main(context, self.filepath)

def export_mot_main(ctx, path):

    anim = open(path, "wb")

    if ctx.selected_objects[0].type == 'ARMATURE':
        arm = ctx.selected_objects[0]
        arm_name = ctx.selected_objects[0].name
    else:
        return {'CANCELLED'}

    action = bpy.data.actions[0]

    animated_pose_bones = []

    for fcurve in action.fcurves:
        pose_bone_path = fcurve.data_path.rpartition('.')[0]
        if pose_bone_path == "":
            animated_pose_bones.append(arm_name)
        else:
            pose_bone = arm.path_resolve(pose_bone_path)
            animated_pose_bones.append(pose_bone.name)

    animated_pose_bones = list(set(animated_pose_bones))

    # take animated_pose_bones list and "sort" it by iterating through the armature's pose.bones.

    animated_pose_bones_cpy = []

    if arm_name in animated_pose_bones:
        animated_pose_bones_cpy.append(arm_name)

    for bone in arm.pose.bones:
        if bone.name in animated_pose_bones:
            animated_pose_bones_cpy.append(bone.name)

    # write MOT header

    frame_start = int(action.frame_range[0])
    framecount = int(action.frame_range[1]) - int(action.frame_range[0]) + 1
    framerate = bpy.context.scene.render.fps

    anim.write(struct.pack(">iIII", -7, 1, framecount, len(animated_pose_bones)))

    for x in range(len(animated_pose_bones)):
        anim.write(struct.pack(">I", len(animated_pose_bones[x])+1))
        anim.write(animated_pose_bones[x].encode()+b"\x00")

    duration = frame_to_ms(framecount, framerate)
    rot_type = 1 # ShortAxis
    pos_type = 0 # Linear
    num_pos_key = 0
    num_rot_key = 0
    num_fov_key = 0

    cars_pos_keys = b""
    cars_rot_keys = b""

    for pbone_name in animated_pose_bones:
        if pbone_name != arm_name:
            pbone = arm.pose.bones[pbone_name]
            cars_pos_keys+=struct.pack(">II", framecount, animated_pose_bones.index(pbone_name))
            num_pos_key+=1
            for frame_idx in range(framecount):
                cars_pos_keys+=get_position_key(ctx, arm, pbone_name, frame_idx, framerate).pack()
            cars_rot_keys+=struct.pack(">II", framecount, animated_pose_bones.index(pbone_name))
            num_rot_key+=1
            for frame_idx in range(framecount):
                cars_rot_keys+=get_rotation_key(ctx, arm, pbone_name, frame_idx, framerate).pack()
        else:
            cars_pos_keys+=struct.pack(">II", framecount, animated_pose_bones.index(pbone_name))
            num_pos_key+=1
            for frame_idx in range(framecount):
                cars_pos_keys+=get_position_key(ctx, arm, arm_name, frame_idx, framerate).pack()
            cars_rot_keys+=struct.pack(">II", framecount, animated_pose_bones.index(pbone_name))
            num_rot_key+=1
            for frame_idx in range(framecount):
                cars_rot_keys+=get_rotation_key(ctx, arm, arm_name, frame_idx, framerate).pack()

    anim.write(struct.pack(">fIIIII", duration, rot_type, pos_type, num_pos_key, num_rot_key, num_fov_key))

    anim.seek(4, 1)
    anim.write(cars_pos_keys)
    anim.write(cars_rot_keys)
    anim.close()
    
    return {'FINISHED'}

def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="GOT/WOT (.got)")

def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()