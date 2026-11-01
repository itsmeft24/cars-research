import hashlib, struct

f = open("cars_mater.exe", "r+b")
data = f.read()
hash = hashlib.md5(data).hexdigest()
f.seek(0)
if hash != '6f008b3c8c6285cf5e2a5688072b96ed':
    print('Invalid EXE! Either this file has already been patched, or it is not Cars: Mater-National!')
    f.close()
    exit(-1)

# Enable Large-Address Awareness.
f.seek(0x11E)
f.write(b"\x23")

# Set the shader model used when compiling shaders at runtime to vs_3_0 and ps_3_0.
f.seek(0x27680B)
f.write(b"3")
f.seek(0x27688F)
f.write(b"3")

# Change the default turntable rotation to match the new FrontEnd scene for the CSS.
f.seek(0x291dbc)
f.write(struct.pack('<f', 0.342018991708756))
f.seek(0x291dc0)
f.write(struct.pack('<f', -0.939692974090576))

# Replace binkw32.dll with CarsGDF.dll, so hooks can initialize before WinMain.
f.seek(0x2e26e0)
f.write(b"CarsGDF.dll\x00")

f.close()