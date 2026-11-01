#pragma once

#include <iostream>
#include <vector>
#include <string>

#define ALLOC_STEP 0x2000

class UniqueStringTable {
	std::vector<std::pair<std::string, std::uint64_t>> strings;
public:
	UniqueStringTable();
	~UniqueStringTable();
	int AddString(const std::string&);
	void RemoveString(const std::string&);
	int LookUpString(const std::string&);
	int GetStringNum();
	std::string GetString(int);
};