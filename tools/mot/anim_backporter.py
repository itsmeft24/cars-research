import struct, sys
from MOTLib import *

anim = open(sys.argv[1], "rb")

anim_out = open(sys.argv[2], "wb")

hdr = struct.unpack(">iIII", anim.read(16))
version = hdr[0]
mot_type = hdr[1]
framecount = hdr[2]
object_count = hdr[3]

assert version == -8

anim_out.write(struct.pack(">iIII", -7, mot_type, framecount, object_count))

objects = []
for x in range(object_count):
    len = struct.unpack(">I", anim.read(4))[0]
    anim_out.write(struct.pack(">I", len))
    objects.append(anim.read(len)[:-1].decode())
    anim_out.write(objects[x].encode()+b"\x00")

hdr_after_table = struct.unpack(">fIIIII", anim.read(24))

duration = hdr_after_table[0]
rot_type = hdr_after_table[1] # force SRK
pos_type = hdr_after_table[2]
num_pos_key = hdr_after_table[3]
num_rot_key = hdr_after_table[4]
num_fov_key = hdr_after_table[5]

out_rot_type = 1

if rot_type == 0:
    out_rot_type = 0

anim_out.write(struct.pack(">fIIIII", duration, out_rot_type, pos_type, num_pos_key, num_rot_key, num_fov_key))
anim_out.write(b"\x00\x00\x00\x00")
anim.seek(4, 1)

for x in range(num_pos_key):
    POSTABLE = struct.unpack(">II", anim.read(8))
    number_of_frames_in_key = POSTABLE[0]
    object_index = POSTABLE[1]
    print(objects[object_index])
    
    anim_out.write(struct.pack(">II", number_of_frames_in_key, object_index))
    
    for x in range(number_of_frames_in_key):
        if pos_type == 1:
            key = BezierPositionKey(anim.read(40))
            anim_out.write(key.pack())
        elif pos_type == 0:
            key = LinearPositionKey(anim.read(16))
            anim_out.write(key.pack())
        print(key)

for x in range(num_rot_key):
    ROTTABLE = struct.unpack(">II", anim.read(8))
    number_of_frames_in_key = ROTTABLE[0]
    object_index = ROTTABLE[1]
    print(objects[object_index])
    
    anim_out.write(struct.pack(">II", number_of_frames_in_key, object_index))
    
    for x in range(number_of_frames_in_key):
        if rot_type == 3:
            key = HalfQuaternionRotationKey(anim.read(12))
            anim_out.write(key.as_srk().pack())
        elif rot_type == 2:
            key = QuaternionRotationKey(anim.read(20))
            anim_out.write(key.as_srk().pack())
        elif rot_type == 1:
            key = ShortRotationKey(anim.read(12))
            anim_out.write(key.pack())
        elif rot_type == 0:
            key = ByteRotationKey(anim.read(8))
            anim_out.write(key.pack())
        print(key)


for x in range(num_fov_key):
    FOVTABLE = struct.unpack(">I", anim.read(4))
    number_of_frames_in_key = FOVTABLE[0]
    object_index = 0
    print(objects[object_index])
    
    anim_out.write(struct.pack(">I", number_of_frames_in_key))
    
    for x in range(number_of_frames_in_key):
        key = FOVKey(anim.read(8))
        anim_out.write(key.pack())
        print(key)

print(anim.tell())
print(anim_out.tell())
anim.close()
anim_out.close()
