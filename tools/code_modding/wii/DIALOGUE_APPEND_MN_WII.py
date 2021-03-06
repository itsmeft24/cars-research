# this script generates an gctrealmate assembly edit to the NTSC-U version of Cars: Mater-National on Wii to allow the user to
# give new playable characters CSS voiceclips.

address_ranges = [
    (0x800001B0, 0x800001FF), 
    (0x800002B0, 0x800002FF), 
    (0x800003B0, 0x800003FF), 
    (0x800004B0, 0x800004FF), 
    (0x800005B0, 0x800005FF), 
    (0x800006B0, 0x800006FF), 
    (0x800007B0, 0x800007FF), 
    (0x800008B0, 0x800008FF), 
    (0x800009B0, 0x80000BFF), 
    (0x80000CB0, 0x80000CFF), 
    (0x80000DB0, 0x80000EFF), 
    (0x80000FB0, 0x80000FFF), 
    (0x800011C0, 0x800012FF), 
    (0x800013B0, 0x800013FF), 
    (0x800014B0, 0x800016FF), 
    (0x800017B0, 0x800017FF)
]

list = open("css_append_mn.txt", "r").read().split("\n")

sum_of_previous_characters = 0
address_range_index = 0

out = open("out_mn.asm", "w")
out.write("Add characters to the Dialogue List in CarsActivityUI::RequestDialogue (GENERATED BY DIALOGUE_APPEND_MN_WII.PY) [itsmeft24]\n")

string_addresses = []

for character in list:
    if address_ranges[address_range_index][0]+sum_of_previous_characters not in range(*address_ranges[address_range_index]):
        address_range_index+=1
        sum_of_previous_characters = 0
    
    string_addr = address_ranges[address_range_index][0]+sum_of_previous_characters
    
    out.write("string " + '"'+character+'"' + " @ $"+hex(string_addr)[2:]+"\n")
    
    sum_of_previous_characters+=len(character)+1
    
    string_addresses.append(string_addr)

out.write("\nHOOK @ $8005698c\n{\n")

for x in range(len(string_addresses)):
    
    out.write("\tmr r3, r30\n")
    out.write("\t\n")
    out.write("\tlis r4, 0x8000 " + '# Load address of "'+list[x]+'" into r4 instead of offseting from r31.' + "\n")
    out.write("\tori r4, r4, 0x" + hex(string_addresses[x])[-4:] + "\n")
    out.write("\t\n")
    out.write("\tlis r12, 0x8004 # BL 800449c4 (CarsActivity::AddNameToDialogueList)\n")
    out.write("\tori r12, r12, 0x49c4\n")
    out.write("\tmtctr r12\n")
    out.write("\tbctrl\n")
    out.write("\t\n")
    out.write("\t\n")


out.write("\tlis r12, 0x8005 # B 800569bc Go back to where the original code is.\n")
out.write("\tori r12, r12, 0x69bc\n")
out.write("\tmtctr r12\n")
out.write("\tbctr\n")
out.write("}")
