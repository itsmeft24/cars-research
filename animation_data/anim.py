import struct, os, math

def ms_to_frame(ms):
    return round(ms / (1/30))

def denormalize_from_1f_to_i16(f):
    return round(f * 32767.0)

def normalize_from_i16_to_1f(uint16):
    return (uint16 / 32767.0) * 1.0

def normalize_from_i16_to_360f(uint16): # useless because this game uses angle-axis
    return (uint16 / 32767.0) * 180.0

class BezierPositionKey:
    def __init__(self, bytes):
        key = struct.unpack(">ffffffffff", bytes) # 40 bytes
        self.Time = key[0]
        self.PosX = key[1]
        self.PosY = key[2]
        self.PosZ = key[3]
        self.TangentInX = key[4]
        self.TangentInY = key[5]
        self.TangentInZ = key[6]
        self.TangentOutX = key[4]
        self.TangentOutY = key[5]
        self.TangentOutZ = key[6]
    def pack(self):
        return struct.pack(">ffffffffff", self.Time, self.PosX, self.PosY, self.PosZ, self.TangentInX, self.TangentInY, self.TangentInZ, self.TangentOutX, self.TangentOutY, self.TangentOutZ)
    def new(time, posx, posy, posz, tinx, tiny, tinz, toutx, touty, toutz):
        return BezierPositionKey(struct.pack(">ffffffffff", time, posx, posy, posz, tinx, tiny, tinz, toutx, touty, toutz))
    def __repr__(self):
        return "BPK: Time: "+str(self.Time)+" PosX:"+str(self.PosX)+" PosY:"+str(self.PosY)+" PosZ:"+str(self.PosZ)

class LinearPositionKey:
    def __init__(self, bytes):
        key = struct.unpack(">ffff", bytes) # 16 bytes
        self.Time = key[0]
        self.PosX = key[1]
        self.PosY = key[2]
        self.PosZ = key[3]
    def pack(self):
        return struct.pack(">ffff", self.Time, self.PosX, self.PosY, self.PosZ)
    def new(time, posx, posy, posz):
        return LinearPositionKey(struct.pack(">ffff", time, posx, posy, posz))
    def __repr__(self):
        return "LPK: Time: "+str(self.Time)+" PosX:"+str(self.PosX)+" PosY:"+str(self.PosY)+" PosZ:"+str(self.PosZ)

class ShortRotationKey:
    def __init__(self, bytes):
        key = struct.unpack(">fhhhh", bytes) # 12 bytes
        self.Time = key[0]
        self.Angle = key[1]
        self.AxisX = key[2]
        self.AxisY = key[3]
        self.AxisZ = key[4]
    def pack(self):
        return struct.pack(">fhhhh", self.Time, self.Angle, self.AxisX, self.AxisY, self.AxisZ)
    def new(time, angle, axx, axy, axz):
        return ShortRotationKey(struct.pack(">fhhhh", time, angle, axx, axy, axz))
    def __repr__(self):
        return "SRK: Time: "+str(self.Time)+" Angle:"+str(self.Angle)+" AxisX:"+str(self.AxisX)+" AxisY:"+str(self.AxisY)+" AxisZ:"+str(self.AxisZ)

class FloatRotationKey: # Exclusive to RoR.
    def __init__(self, bytes):
        key = struct.unpack(">fffff", bytes) # 20 bytes
        self.Time = key[0]
        self.Angle = key[1]
        self.AxisX = key[2]
        self.AxisY = key[3]
        self.AxisZ = key[4]
    def pack(self):
        return struct.pack(">fffff", self.Time, self.Angle, self.AxisX, self.AxisY, self.AxisZ)
    def new(time, angle, axx, axy, axz):
        return ShortRotationKey(struct.pack(">fffff", time, angle, axx, axy, axz))
    def __repr__(self):
        return "FRK: Time: "+str(self.Time)+" Angle:"+str(self.Angle)+" AxisX:"+str(self.AxisX)+" AxisY:"+str(self.AxisY)+" AxisZ:"+str(self.AxisZ)

class ByteRotationKey:
    def __init__(self, bytes):
        key = struct.unpack(">fbbbb", bytes) # 8 bytes
        self.Time = key[0]
        self.Angle = key[1]
        self.AxisX = key[2]
        self.AxisY = key[3]
        self.AxisZ = key[4]
    def pack(self):
        return struct.pack(">fbbbb", self.Time, self.Angle, self.AxisX, self.AxisY, self.AxisZ)
    def new(time, angle, axx, axy, axz):
        return ByteRotationKey(struct.pack(">fbbbb", time, angle, axx, axy, axz))
    def __repr__(self):
        return "BRK: Time: "+str(self.Time)+" Angle:"+str(self.Angle)+" AxisX:"+str(self.AxisX)+" AxisY:"+str(self.AxisY)+" AxisZ:"+str(self.AxisZ)

class FOVKey:
    def __init__(self, bytes):
        key = struct.unpack(">ff", bytes) # 8 bytes
        self.Time = key[0]
        self.FOV = key[1]
    def pack(self):
        return struct.pack(">ff", self.Time, self.FOV)
    def new(time, angle, axx, axy, axz):
        return FOVKey(struct.pack(">ff", Time, FOV))
    def __repr__(self):
        return "FOV: Time: "+str(self.Time)+" FOV:"+str(self.FOV)

def frk_to_srk(FRK):
    return ShortRotationKey.new(
        FRK.Time,
        denormalize_from_1f_to_i16(FRK.Angle),
        denormalize_from_1f_to_i16(FRK.AxisX),
        denormalize_from_1f_to_i16(FRK.AxisY),
        denormalize_from_1f_to_i16(FRK.AxisZ)
    )

anim = open("CAN_BEF_BREATH_B.got", "rb")


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

print("PositionType: "+str(pos_type))
print("RotationType: "+str(rot_type))

anim.seek(4, 1)

pos_arr = []
rot_arr = []
fov_arr = []

print(anim.tell())

for x in range(num_pos_key):
    POSTABLE = struct.unpack(">II", anim.read(8))
    number_of_frames_in_key = POSTABLE[0]
    object_index = POSTABLE[1]
    print(objects[object_index])
    
    for x in range(number_of_frames_in_key):
        if version == -7:
            
            if pos_type == 1:
                key = BezierPositionKey(anim.read(40))
                pos_arr.append(key)
            elif pos_type == 0:
                key = LinearPositionKey(anim.read(16))
                pos_arr.append(key)
            
        elif version == -8:
            
            if pos_type == 1:
                key = BezierPositionKey(anim.read(40))
                pos_arr.append(key)
            elif pos_type == 0:
                key = LinearPositionKey(anim.read(16))
                pos_arr.append(key)
            
        print(key)

print(anim.tell())

for x in range(num_rot_key):
    ROTTABLE = struct.unpack(">II", anim.read(8))
    number_of_frames_in_key = ROTTABLE[0]
    object_index = ROTTABLE[1]
    print(objects[object_index])
    
    for x in range(number_of_frames_in_key):
        if version == -7:
            
            if rot_type == 1:
                key = ShortRotationKey(anim.read(12))
                rot_arr.append(key)
            elif rot_type == 0:
                key = ByteRotationKey(anim.read(8))
                rot_arr.append(key)
            
        elif version == -8:
        
            if rot_type == 2:
                key = FloatRotationKey(anim.read(20))
                rot_arr.append(key)
            elif rot_type == 0 or rot_type == 1 or rot_type == 3:
                key = ShortRotationKey(anim.read(12))
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

print(anim.tell())
os.system("pause")
