#include <algorithm>
#include <iostream>
#include "../xbr.h"
#include <vector>
#include <fstream>
#include <filesystem>

void replace_all_in_place(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

void write_null(std::ostream& stream, unsigned int size) {
    unsigned int x = 0;
    char null = '\x00';

    while (x < size) {
        stream.write(&null, 1);
        x++;
    }
}

unsigned long pad_to_800(unsigned long size) {
    if ((size / 0x800) * 0x800 == size)
        return size;
    return (size / 0x800 + 1) * 0x800;
}

struct fileinfo {
    std::string path;
    unsigned long hash;
    unsigned long size;
    unsigned long padded_size() {
        return pad_to_800(size);
    }
};

bool fileinfo_compare(const fileinfo& a, const fileinfo& b)
{
    return a.hash < b.hash;
}

int main(int argc, char* argv[])
{
    std::ofstream XBR("cars3_new.xbr", std::ios::out | std::ios::binary);
    
    std::vector<fileinfo> files;

    std::string path = "E:\\CarsResearch\\RoR360\\installer\\release";
    std::cout << "Collecting files in " << path << "..." << std::endl;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (entry.is_regular_file()) {

            std::string xbrpath = entry.path().string();

            replace_all_in_place(xbrpath, path + "\\", "");

            files.push_back(
                fileinfo{ entry.path().string(), Cars3ARC::djb_hash(xbrpath), (unsigned long)std::filesystem::file_size(entry.path()) }
            );
        }
    }

    std::sort(files.begin(), files.end(), fileinfo_compare);

    Cars3ARC::XBR_HEADER hdr{3, files.size(), 0, 0, files.size() * 12, 0};
    hdr.SWAP();

    XBR.write((char *)&hdr, sizeof(hdr));
    unsigned long sum_of_previous_sizes = pad_to_800(files.size() * 12 + 24); // size of header + size of table
    std::cout << "Writing files..." << std::endl;
    for (unsigned long x = 0; x < files.size(); x++) {
        XBR.seekp(x * 12 + 24);
        Cars3ARC::XBR_TABLE_ENTRY table_entry{ files[x].hash, files[x].size, sum_of_previous_sizes / 0x800};
        table_entry.SWAP();

        XBR.write((char*)&table_entry, sizeof(table_entry));
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