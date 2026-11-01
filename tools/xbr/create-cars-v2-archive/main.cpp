#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

struct PSRHeader {
    std::uint32_t version;
    std::uint32_t number_files;
    std::uint32_t unk;
    std::uint32_t unk1;
    std::uint32_t table_size;
    std::uint32_t string_table_len;
};

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

constexpr std::uint32_t div_round_up(std::uint32_t size) {
    if ((size / 0x800) * 0x800 == size)
        return size;
    return (size / 0x800 + 1) * 0x800;
}

struct FileInfo {
    std::filesystem::path absolute_path;
    std::string relative_path;
    std::uint32_t size;
};

void print_help(std::string argstr) {
    std::cout << "Cars Mater-National / The Video Game .XBR/.PSR Packer" << std::endl << std::endl;
    std::cout << "Usage: " << argstr << " <FOLDER_PATH> <XBR_PSR_FILE>" << std::endl << std::endl;
}

// Custom comparator for std::sort
bool custom_compare(const FileInfo& a, const FileInfo& b) {
    return a.relative_path.compare(b.relative_path) < 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_help(argv[0]);
        return -1;
    }

    std::ofstream XBR(argv[2], std::ios::out | std::ios::binary);
    if (!XBR.is_open()) {
        std::cout << "Failed to open " << argv[2] << ".";
        return -1;
    }

    std::vector<FileInfo> files;
    std::filesystem::path root_path = argv[1];

    std::cout << "Collecting files in " << root_path << "..." << std::endl;

    std::uint32_t output_table_size = 0;
    std::uint64_t max_file_size = 0;
    std::uint32_t string_size = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root_path)) {
        if (entry.is_regular_file()) {

            std::filesystem::path absolute_path = entry.path();
            std::string relative_path = absolute_path.lexically_relative(root_path).string();
            std::uint32_t size = std::filesystem::file_size(entry.path());

            if (size > max_file_size) {
                max_file_size = size;
            }

            files.emplace_back(absolute_path, relative_path, size);
            string_size += relative_path.size() + 1;
            output_table_size += relative_path.size() + 1 + 12;
        }
    }

    std::sort(files.begin(), files.end(), custom_compare);

    PSRHeader hdr{ 2, files.size(), 0x34325446, 0x34325446, output_table_size, string_size };
    XBR.write(reinterpret_cast<char*>(&hdr), sizeof(PSRHeader));
    
    char* work_buffer = new char[div_round_up(max_file_size)](0);
    std::uint32_t chunk_offset = div_round_up(output_table_size + sizeof(PSRHeader));
    std::streamoff entry_offset = 24;
    std::cout << "Writing files..." << std::endl;
    for (std::size_t i = 0; i < files.size(); i++) {
        XBR.seekp(entry_offset);
        
        std::uint32_t str_size = files[i].relative_path.size();
        std::uint32_t offset_shifted = chunk_offset / 0x800;
        XBR.write(reinterpret_cast<char*>(&str_size), sizeof(std::uint32_t));
        XBR.write(reinterpret_cast<char*>(files[i].relative_path.data()), files[i].relative_path.size());
        XBR.write(reinterpret_cast<char*>(&files[i].size), sizeof(std::uint32_t));
        XBR.write(reinterpret_cast<char*>(&offset_shifted), sizeof(std::uint32_t));

        XBR.seekp(chunk_offset);

        std::ifstream file_read(files[i].absolute_path, std::ios::in | std::ios::binary);
        file_read.read(work_buffer, files[i].size);
        file_read.close();
        XBR.write(work_buffer, files[i].size);

        entry_offset = entry_offset + str_size + 12;
        chunk_offset = chunk_offset + div_round_up(files[i].size);
        std::cout << "Added " << files[i].relative_path << '\n';
    }

    write_null(XBR, div_round_up(XBR.tellp()) - XBR.tellp());

    XBR.close();
    delete[] work_buffer;
    std::cout << "Done." << std::endl;
}