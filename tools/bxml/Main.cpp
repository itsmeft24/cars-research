
#include <filesystem>
#include "XMLTool.h"

int main(int argc, char* argv[]) {
	if (argc < 4) {
		printf("modified version of XMLTool (updated from BXML version 1000 to 1002), made with love by itsmeft24\n");
		printf("Usage: %s <Mode> <Input File> <Output File>\n", argv[0]);
		printf("Modes:\n");
		printf("\t-disasm: Disassemble a BXML into an XML.\n");
		printf("\t-asm: Assemble an XML into a BXML.\n");
		return -1;
	}
	std::unique_ptr<XMLTool> tool = std::make_unique<XMLTool>();
	if (_strnicmp(argv[1], "-disasm", 7) == 0) {
		if (tool->LoadFromBXML(argv[2])) {
			if (!tool->WriteXML(argv[3])) {
				printf("failed to open output file.\n");
			}
		}
		else {
			printf("failed to parse or open input file.\n");
		}
	}
	else if (_strnicmp(argv[1], "-asm", 4) == 0) {
		if (tool->LoadFromXML(argv[2])) {
			if (!tool->WriteBinaryXML(argv[3])) {
				printf("failed to open output file.\n");
			}
		}
		else {
			printf("failed to parse or open input file.\n");
		}
	}
}