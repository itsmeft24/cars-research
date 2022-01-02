#include <vector>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <string>

namespace Cars3ARC {

	std::unordered_map<unsigned long, std::string> labels;

	unsigned long djb_hash(std::string str)
	{	
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });

		unsigned long hash = 5381;

		for (size_t x = 0; x < str.size(); x++)
		{
			//hash = ((hash << 5) + hash) + str[x];
			hash = hash * 0x21 + str[x];
		}

		return hash;
	}

	bool init_hashes(std::string hash_path) {
		std::ifstream hash_labels;
		std::string line;
		hash_labels.open(hash_path, std::ios::in);
		if (!hash_labels) {
			return false;
		}
		while (std::getline(hash_labels, line)) {
			labels.insert({ djb_hash(line), line });
		}
		hash_labels.close();
		return true;
	}

	void swap_ulong(unsigned long* x) {
		*x = (*x & 0x0000FFFF) << 16 | (*x & 0xFFFF0000) >> 16;
		*x = (*x & 0x00FF00FF) << 8 | (*x & 0xFF00FF00) >> 8;
	}

	#pragma pack(push, 1)

	struct XBR_HEADER {
		unsigned long version;
		unsigned long number_files;
		unsigned long unk;
		unsigned long unk1;
		unsigned long table_size;
		unsigned long padding;
		void SWAP() {
			swap_ulong(&version);
			swap_ulong(&number_files);
			swap_ulong(&unk);
			swap_ulong(&unk1);
			swap_ulong(&table_size);
			swap_ulong(&padding);
		}
	};

	struct XBR_TABLE_ENTRY {
		unsigned long hash;
		unsigned long size;
		unsigned long offset;
		void SWAP() {
			swap_ulong(&hash);
			swap_ulong(&size);
			swap_ulong(&offset);
		}
	};

	struct XBR_RESULT {
		unsigned long size;
		unsigned long offset;
		void SWAP() {
			swap_ulong(&size);
			swap_ulong(&offset);
		}
	};

	#pragma pack(pop)

	class XBR_FILE {
	public:
		XBR_HEADER header;
		std::vector<XBR_TABLE_ENTRY> table;
		std::ifstream file;
		bool is_ps3;
		bool read(std::string path, bool is_ps3_) {
			is_ps3 = is_ps3_;
			file.open(path, std::ios::in | std::ios::binary);
			if (!file) {
				return false;
			}
			file.read(reinterpret_cast<char*>(&header), sizeof(header));
			header.SWAP();
			table.reserve(header.number_files);
			XBR_TABLE_ENTRY curr_entry;
			for (int x = 0; x < header.number_files; x++) {
				file.read(reinterpret_cast<char*>(&curr_entry), sizeof(curr_entry));
				curr_entry.SWAP();
				table.push_back(curr_entry);
			}
			return true;
		}
		bool lookup(std::string path, XBR_TABLE_ENTRY* result) {
			unsigned long hash = djb_hash(path);
			for (int x = 0; x < header.number_files; x++) {
				if (table[x].hash == hash) {
					/*
					result = &(table[x]);
					return true;
					*/
					(*result).hash = table[x].hash;
					(*result).size = table[x].size;
					if (is_ps3) {
						(*result).offset = table[x].offset;
					}
					else {
						(*result).offset = table[x].offset << 11; // * 0x800
					}
					return true;
				}
			}
			return false;
		}
	};

}