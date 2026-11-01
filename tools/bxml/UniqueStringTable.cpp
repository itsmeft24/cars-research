
#include <algorithm>

#include "UniqueStringTable.h"

UniqueStringTable::UniqueStringTable()
{
}

UniqueStringTable::~UniqueStringTable()
{
}

int UniqueStringTable::AddString(
   const std::string& pString
)
{
    const auto& it = std::find_if(strings.begin(), strings.end(), [&](const std::pair<std::string, std::uint64_t>& val) -> bool {return val.first == pString;});
    if (it != strings.end()) {
        // inc refcount
        it->second++;
        return it - strings.begin();
    }
    strings.push_back(std::make_pair(pString, 1));

    //sort list
    std::sort(strings.begin(), strings.end());
    return std::find_if(strings.begin(), strings.end(), [&](const std::pair<std::string, std::uint64_t>& val) -> bool {return val.first == pString; }) - strings.begin();
}

void UniqueStringTable::RemoveString(const std::string& pString)
{
    const auto& it = std::find_if(strings.begin(), strings.end(), [&](const std::pair<std::string, std::uint64_t>& val) -> bool {return val.first == pString; });
    if (it == strings.end()) {
        return;
    }
    it->second--;
    if (it->second == 0) {
        strings.erase(it);
    }
}

int UniqueStringTable::LookUpString(const std::string& pString)
{
    return std::find_if(strings.begin(), strings.end(), [&](const std::pair<std::string, std::uint64_t>& val) -> bool {return val.first == pString; }) - strings.begin();
}

int UniqueStringTable::GetStringNum()
{
   return strings.size();
}

std::string UniqueStringTable::GetString(
   int index
)
{
   if (index < 0 || index > strings.size()) {
      return "";
   }

   return strings.at(index).first;
}

