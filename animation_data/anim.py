import struct
from MOTLib import *

anim = open(r"../Standin_DustDevil_1.got", "rb")

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

pos_arr = []
rot_arr = []
fov_arr = []

for x in range(num_pos_key):
    POSTABLE = struct.unpack(">II", anim.read(8))
    number_of_frames_in_key = POSTABLE[0]
    object_index = POSTABLE[1]
    print(objects[object_index])
    
    for x in range(number_of_frames_in_key):
        
        if pos_type == 1:
            key = BezierPositionKey(anim.read(40))
            pos_arr.append(key)
        elif pos_type == 0:
            key = LinearPositionKey(anim.read(16))
            pos_arr.append(key)
        
        print(key)

for x in range(num_rot_key):
    ROTTABLE = struct.unpack(">II", anim.read(8))
    number_of_frames_in_key = ROTTABLE[0]
    object_index = ROTTABLE[1]
    print(objects[object_index])
    
    for x in range(number_of_frames_in_key):
        
        if rot_type == 3:
            key = HalfQuaternionRotationKey(anim.read(12))
            rot_arr.append(key)
        if rot_type == 2:
            key = QuaternionRotationKey(anim.read(20))
            rot_arr.append(key)
        elif rot_type == 1:
            key = ShortRotationKey(anim.read(12))
            rot_arr.append(key)
        elif rot_type == 0:
            key = ByteRotationKey(anim.read(8))
            rot_arr.append(key)
        
        print(key)

for x in range(num_fov_key):
    FOVTABLE = struct.unpack(">I", anim.read(4))
    number_of_frames_in_key = FOVTABLE[0]
    object_index = 0
    print(objects[object_index])
    
    for x in range(number_of_frames_in_key):
        key = FOVKey(anim.read(8))
        fov_arr.append(key)
        print(key)