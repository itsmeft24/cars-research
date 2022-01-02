#include "xbr.h"
#include <filesystem>

std::string get_parent(std::string FILE_PATH, bool is_win) {
	char delimiter;
	if (is_win) {
		delimiter = '\\';
	}
	else {
		delimiter = '/';
	}
	int start_of_file = FILE_PATH.find_last_of(delimiter);
	return FILE_PATH.substr(0, start_of_file);
}

void print_help(std::string argstr) {
	std::cout << "Cars Race-O-Rama .XBR/.P3R File Extractor" << std::endl << std::endl;
	std::cout << "Usage: " << argstr << " <XBR_P3R_FILE> <FILE_PATH> <PLATFORM>" << std::endl << std::endl;
	std::cout << "Platforms: -x360\t Xbox 360" << std::endl;
	std::cout << "           -ps3 \t PlayStation 3";
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		print_help(argv[0]);
		return -1;
	}
	if (!(strcmp(argv[3], "-x360") != 0) && !(strcmp(argv[3], "-ps3") != 0)){
		std::cout << argv[3];
		print_help(argv[0]);
		return -1;
	}

	std::string XBR_PATH = argv[1];
	std::string FILE_PATH = argv[2];
	std::string PLATFORM = argv[3];
	std::string PARENT = FILE_PATH;

	Cars3ARC::XBR_FILE xbr;

	if (PLATFORM == "-ps3") { // Force Linux-ify the path.
		std::replace(FILE_PATH.begin(), FILE_PATH.end(), '\\', '/');
		PARENT = get_parent(FILE_PATH, false);
		if (!xbr.read(XBR_PATH, true)) {
			std::cout << "Failed to open archive.";
			return -1;
		}
	}

	else { // Force Windows-ify the path.
		std::replace(FILE_PATH.begin(), FILE_PATH.end(), '/', '\\');
		PARENT = get_parent(FILE_PATH, true);
		if (!xbr.read(XBR_PATH, false)) {
			std::cout << "Failed to open archive.";
			return -1;
		}
	}

	Cars3ARC::XBR_TABLE_ENTRY xbr_entry;

	if (!xbr.lookup(FILE_PATH, &xbr_entry)) {
		std::cout << FILE_PATH << " Was not found in " << XBR_PATH << "." << std::endl;
		return -1;
	}

	xbr.file.seekg(xbr_entry.offset);

	char* bytes = new char[xbr_entry.size];

	xbr.file.read(bytes, xbr_entry.size);

	std::filesystem::create_directories(PARENT);

	std::ofstream output;
	output.open(FILE_PATH, std::ios::out | std::ios::binary);
	output.write(bytes, xbr_entry.size);
	output.close();
	delete[] bytes;
	std::cout << "Extracted File: " << FILE_PATH << std::endl << "DJB-2 Hash: 0x" << std::hex << xbr_entry.hash << std::endl;

}