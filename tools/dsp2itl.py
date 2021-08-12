import os, math, sys

Lchannel = open(sys.argv[1], "r+b")
Rchannel = open(sys.argv[2], "r+b")

Lchannel.seek(0, 2)
LSIZE = Lchannel.tell()
Lchannel.seek(0)

Rchannel.seek(0, 2)
RSIZE = Rchannel.tell()
Rchannel.seek(0)

L_CHUNKS = math.floor(LSIZE / 0x10000)
L_EXTDATA = LSIZE % 0x10000

Lchannel.seek(LSIZE - L_EXTDATA)
L_EXTDATA = Lchannel.read(L_EXTDATA)

R_CHUNKS = math.floor(RSIZE / 0x10000)
R_EXTDATA = RSIZE % 0x10000

Rchannel.seek(RSIZE - R_EXTDATA)
R_EXTDATA = Rchannel.read(R_EXTDATA)

DATA_OUT = open(sys.argv[3], "w+b")
DATA_OUT.write((LSIZE+RSIZE)*b"\x00")

Lchannel.seek(0)
Rchannel.seek(0)

for x in range(L_CHUNKS):
    DATA_OUT.seek(x*0x20000)
    LDATA = Lchannel.read(0x10000)
    RDATA = Rchannel.read(0x10000)
    DATA_OUT.write(LDATA)
    DATA_OUT.write(RDATA)
DATA_OUT.write(L_EXTDATA)
DATA_OUT.write(R_EXTDATA)
