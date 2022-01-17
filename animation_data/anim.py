import struct, os, math, binascii

def ms_to_frame(ms):
    return round(ms / (1/30))

def denormalize_from_1f_to_i16(f):
    return round(f * 32767.0)

def normalize_from_i16_to_1f(int16):
    return (int16 / 32767.0) * 1.0

def denormalize_from_1f_to_i8(f):
    return round(f * 127.0)

def normalize_from_i8_to_1f(int8):
    return (int8 / 127.0) * 1.0

# https://gamedev.stackexchange.com/questions/28023/python-float-32bit-to-half-float-16bit
def f16_to_f32(float16):
    s = int((float16 >> 15) & 0x00000001)    # sign
    e = int((float16 >> 10) & 0x0000001f)    # exponent
    f = int(float16 & 0x000003ff)            # fraction

    if e == 0:
        if f == 0:
            ret = int(s << 31)
            return struct.unpack(">f", ret.to_bytes(4, "big"))[0]
        else:
            while not (f & 0x00000400):
                f = f << 1
                e -= 1
            e += 1
            f &= ~0x00000400
            #print(s,e,f)
    elif e == 31:
        if f == 0:
            ret = int((s << 31) | 0x7f800000)
            return struct.unpack(">f", ret.to_bytes(4, "big"))[0]
        else:
            ret = int((s << 31) | 0x7f800000 | (f << 13))
            return struct.unpack(">f", ret.to_bytes(4, "big"))[0]

    e = e + (127 -15)
    f = f << 13
    ret = int((s << 31) | (e << 23) | f)
    return struct.unpack(">f", ret.to_bytes(4, "big"))[0]

def f32_to_f16(float32):
    F16_EXPONENT_BITS = 0x1F
    F16_EXPONENT_SHIFT = 10
    F16_EXPONENT_BIAS = 15
    F16_MANTISSA_BITS = 0x3ff
    F16_MANTISSA_SHIFT =  (23 - F16_EXPONENT_SHIFT)
    F16_MAX_EXPONENT =  (F16_EXPONENT_BITS << F16_EXPONENT_SHIFT)

    a = struct.pack('>f',float32)
    b = binascii.hexlify(a)

    f32 = int(b,16)
    f16 = 0
    sign = (f32 >> 16) & 0x8000
    exponent = ((f32 >> 23) & 0xff) - 127
    mantissa = f32 & 0x007fffff

    if exponent == 128:
        f16 = sign | F16_MAX_EXPONENT
        if mantissa:
            f16 |= (mantissa & F16_MANTISSA_BITS)
    elif exponent > 15:
        f16 = sign | F16_MAX_EXPONENT
    elif exponent > -15:
        exponent += F16_EXPONENT_BIAS
        mantissa >>= F16_MANTISSA_SHIFT
        f16 = sign | exponent << F16_EXPONENT_SHIFT | mantissa
    else:
        f16 = sign
    return f16

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

class QuaternionRotationKey: # Exclusive to ROR.
    def __init__(self, bytes):
        key = struct.unpack(">fffff", bytes) # 12 bytes
        self.Time = key[0]
        self.X = key[1]
        self.Y = key[2]
        self.Z = key[3]
        self.W = key[4]
    def pack(self):
        return struct.pack(">fffff", self.Time, self.X, self.Y, self.Z, self.W)
    def new(time, x, y, z, w):
        return HalfQuaternionRotationKey(struct.pack(">fffff", time, x, y, z, w))
    def __repr__(self):
        return "QRK: Time: "+str(self.Time)+" X:"+str(self.X)+" Y:"+str(self.Y)+" Z:"+str(self.Z)+" W:"+str(self.W)

class HalfQuaternionRotationKey: # Exclusive to ROR.
    def __init__(self, bytes):
        key = struct.unpack(">fhhhh", bytes) # 12 bytes
        self.Time = key[0]
        self.X = f16_to_f32(key[1])
        self.Y = f16_to_f32(key[2])
        self.Z = f16_to_f32(key[3])
        self.W = f16_to_f32(key[4])
    def pack(self):
        return struct.pack(">fhhhh", f32_to_f16(self.Time), f32_to_f16(self.X), f32_to_f16(self.Y), f32_to_f16(self.Z), f32_to_f16(self.W))
    def new(time, x, y, z, w):
        return HalfQuaternionRotationKey(struct.pack(">fhhhh", f32_to_f16(time), f32_to_f16(x), f32_to_f16(y), f32_to_f16(z), f32_to_f16(w)))
    def __repr__(self):
        return "HQRK: Time: "+str(self.Time)+" X:"+str(self.X)+" Y:"+str(self.Y)+" Z:"+str(self.Z)+" W:"+str(self.W)
    def as_qrk(self):
        return QuaternionRotationKey.new(self.Time, self.X, self.Y, self.Z, self.W)

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

anim = open(r"FrontEndUI_Camera_Anim_HD.got", "rb")

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

print(anim.tell())
os.system("pause")
