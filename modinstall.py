import sys, os

modfiles = os.listdir("mods")

xbr = open("Cars3.xbr", "r+b")

for x in range(len(modfiles)):
    modfile = open("./mods/"+modfiles[x], "rb").read()
    offset = int((modfiles[x].split(".")[0]),16)
    xbr.seek(offset)
    xbr.write(modfile)