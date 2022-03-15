#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

#include "../xbr.h"

void replace_all_in_place(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

constexpr void write_null(std::ostream& stream, unsigned int size) {
    for (int x = 0; x < size; x++) {
        stream.put(0);
    }
}

constexpr unsigned long pad_to_800(unsigned long size) {
    if ((size / 0x800) * 0x800 == size)
        return size;
    return (size / 0x800 + 1) * 0x800;
}

struct fileinfo {
    std::string path;
    std::string relative_path;
    unsigned long size;
    unsigned long padded_size() {
        return pad_to_800(size);
    }
};

unsigned long long get_folder_size(std::string PATH) {
    unsigned long long size_ret = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(PATH)) {
        if (entry.is_regular_file()) {
            size_ret = size_ret + (unsigned long long)std::filesystem::file_size(entry.path());
        }
    }

    return size_ret;
}

void print_help(std::string argstr) {
    std::cout << "Cars Mater-National / The Video Game .XBR/.PSR Packer" << std::endl << std::endl;
    std::cout << "Usage: " << argstr << " <FOLDER_PATH> <XBR_PSR_FILE>" << std::endl << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        print_help(argv[0]);
        return -1;
    }

    std::ofstream XBR(argv[2], std::ios::out | std::ios::binary);
    if (!XBR) {
        std::cout << "Failed to open " << argv[2] << ".";
        return -1;
    }

    std::vector<fileinfo> files;

    std::string path = argv[1];

    if (pad_to_800(get_folder_size(path)) > 0xFFFFFFFF) {
        std::cout << "Input folder is too large. Aborting...";
        return -1;
    }

    std::cout << "Collecting files in " << path << "..." << std::endl;

    unsigned long output_table_size = 0;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {

        if (entry.is_regular_file()) {

            std::string xbrpath = entry.path().string();

            replace_all_in_place(xbrpath, path + "\\", "");

            files.push_back(
                fileinfo{ entry.path().string(), xbrpath, (unsigned long)std::filesystem::file_size(entry.path()) }
            );
            output_table_size = output_table_size + 4 + xbrpath.size() + 4 + 4;
        }
    }

    Cars3ARC::XBR_HEADER hdr{ 2, files.size(), 2824, 74400, output_table_size, 702022 };

    XBR.write((char*)&hdr, sizeof(hdr));
    unsigned long sum_of_previous_sizes = pad_to_800(output_table_size + 24); // size of header + size of table
    std::streamoff pos = 24;
    std::cout << "Writing files..." << std::endl;
    for (unsigned long x = 0; x < files.size(); x++) {
        XBR.seekp(pos);
        
        unsigned long str_size = files[x].relative_path.size();
        XBR.write((char *)&str_size, sizeof(str_size));
        XBR.write((char *)&(files[x].relative_path[0]), str_size);

        XBR.write((char*)&(files[x].size), sizeof(files[x].size));
        XBR.write((char*)&sum_of_previous_sizes, sizeof(sum_of_previous_sizes));

        pos = pos + 4 + str_size + 4 + 4;
        XBR.seekp(sum_of_previous_sizes);

        char* bytes = new char[files[x].size];

        std::ifstream file_read(files[x].path, std::ios::in | std::ios::binary);
        file_read.read(bytes, files[x].size);
        file_read.close();
        XBR.write(bytes, files[x].size);
        delete[] bytes;

        sum_of_previous_sizes = sum_of_previous_sizes + files[x].padded_size();
        std::cout << "Added " << files[x].path << std::endl;
    }

    write_null(XBR, pad_to_800(XBR.tellp()) - XBR.tellp());

    XBR.close();
    std::cout << "Done." << std::endl;
}