#include <vector>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <string>

namespace Cars3ARC {

	enum class Platform : uint32_t
	{
		Xbox360,
		PS3,
		PS2
	};

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

	#pragma pack(pop)

	class XBR_FILE {
	private:
		XBR_HEADER m_header;
		std::vector<XBR_TABLE_ENTRY> m_table;
		Platform m_platform;
	public:
		XBR_FILE(std::istream& file, Platform p) {
			m_platform = p;

			file.read((char*)&m_header, sizeof(m_header));

			if (m_platform == Platform::Xbox360 || m_platform == Platform::PS3)
				m_header.SWAP();

			m_table.resize(m_header.number_files);
			file.read((char*)&m_table[0], m_header.number_files * sizeof(XBR_TABLE_ENTRY));

			if (m_platform == Platform::Xbox360 || m_platform == Platform::PS3) {
				for (auto& f : m_table) {
					f.SWAP();
				}
			}

			if (m_platform == Platform::Xbox360 || m_platform == Platform::PS2) {
				for (auto& f : m_table) {
					f.offset <<= 11;
				}
			}
		}

		XBR_TABLE_ENTRY* lookup(std::string path) {
			unsigned long hash = djb_hash(path);
			for (auto& f : m_table) {
				if (f.hash == hash) {
					return &f;
				}
			}
			return nullptr;
		}
	};

}