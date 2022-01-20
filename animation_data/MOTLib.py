import struct, math, binascii
from mathutils import *

BYTE_ANGLE_AXIS_KEY_SCALE = (127.5/math.pi)
SHORT_ANGLE_AXIS_KEY_SCALE = (2048/math.pi)

def ms_to_frame(ms):
    return round(ms / (1/30))

def frame_to_ms(frame_idx, fps):
    return frame_idx*1.0/fps

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
        return QuaternionRotationKey(struct.pack(">fffff", time, x, y, z, w))
    def __repr__(self):
        return "QRK: Time: "+str(self.Time)+" X:"+str(self.X)+" Y:"+str(self.Y)+" Z:"+str(self.Z)+" W:"+str(self.W)
    def as_srk(self):
        quat = Quaternion((self.W, self.X, self.Y, self.Z))
        axis, angle = quat.to_axis_angle()
        axis = axis.normalized()
        return ShortRotationKey.new(
            self.Time,
            round(angle*SHORT_ANGLE_AXIS_KEY_SCALE), # why the h e double hockey sticks this works is beyond me but w/e (constant's value is equal to 2048/pi)
            denormalize_from_1f_to_i16(axis.x),
            denormalize_from_1f_to_i16(axis.y),
            denormalize_from_1f_to_i16(axis.z)
        )

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
    def as_srk(self):
        return self.as_qrk().srk() # i would do some inheritance to make it cleaner but nah

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