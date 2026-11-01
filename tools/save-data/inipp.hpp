/*
MIT License

Copyright (c) 2017-2020 Matthias C. M. Troffaes

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Modified by itsmeft24 for use with assets from Rainbow Studios games.

#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <vector>
#include <locale>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

namespace inipp {

	namespace detail {

		// trim functions based on http://stackoverflow.com/a/217605

		template <class CharT>
		inline void ltrim(std::basic_string<CharT>& s, const std::locale& loc) {
			s.erase(s.begin(),
				std::find_if(s.begin(), s.end(),
					[&loc](CharT ch) { return !std::isspace(ch, loc); }));
		}

		template <class CharT>
		inline void rtrim(std::basic_string<CharT>& s, const std::locale& loc) {
			s.erase(std::find_if(s.rbegin(), s.rend(),
				[&loc](CharT ch) { return !std::isspace(ch, loc); }).base(),
				s.end());
		}

		template <class CharT, class UnaryPredicate>
		inline void rtrim2(std::basic_string<CharT>& s, UnaryPredicate pred) {
			s.erase(std::find_if(s.begin(), s.end(), pred), s.end());
		}

		// string replacement function based on http://stackoverflow.com/a/3418285

		template <class CharT>
		inline bool replace(std::basic_string<CharT>& str, const std::basic_string<CharT>& from, const std::basic_string<CharT>& to) {
			auto changed = false;
			size_t start_pos = 0;
			while ((start_pos = str.find(from, start_pos)) != std::basic_string<CharT>::npos) {
				str.replace(start_pos, from.length(), to);
				start_pos += to.length();
				changed = true;
			}
			return changed;
		}

	} // namespace detail

	template <typename CharT, typename T>
	inline bool extract(const std::basic_string<CharT>& value, T& dst) {
		CharT c;
		std::basic_istringstream<CharT> is{ value };
		T result;
		if ((is >> std::boolalpha >> result) && !(is >> c)) {
			dst = result;
			return true;
		}
		else {
			return false;
		}
	}

	template <typename CharT>
	inline bool extract(const std::basic_string<CharT>& value, std::basic_string<CharT>& dst) {
		dst = value;
		return true;
	}

	template <typename CharT, typename T>
	inline bool get_value(const std::map<std::basic_string<CharT>, std::basic_string<CharT>>& sec, const std::basic_string<CharT>& key, T& dst) {
		const auto it = sec.find(key);
		if (it == sec.end()) return false;
		return extract(it->second, dst);
	}

	template <typename CharT, typename T>
	inline bool get_value(const std::map<std::basic_string<CharT>, std::basic_string<CharT>>& sec, const CharT* key, T& dst) {
		return get_value(sec, std::basic_string<CharT>(key), dst);
	}

	template<class CharT>
	class Format
	{
	public:
		// used for generating
		const CharT char_section_start;
		const CharT char_section_end;
		const CharT char_assign;
		const CharT char_comment;

		// used for parsing
		virtual bool is_section_start(CharT ch) const { return ch == char_section_start; }
		virtual bool is_section_end(CharT ch) const { return ch == char_section_end; }
		virtual bool is_assign(CharT ch) const { return ch == char_assign; }
		virtual bool is_comment(CharT ch) const { return ch == char_comment; }

		Format(CharT section_start, CharT section_end, CharT assign, CharT comment)
			: char_section_start(section_start)
			, char_section_end(section_end)
			, char_assign(assign)
			, char_comment(comment) {}

		Format() : Format('[', ']', '=', ';') {}
	};


	template<class CharT>
	class _Ini
	{
	public:
		using String = std::basic_string<CharT>;
		class Section {
		public:
			std::vector<std::pair<String, String>> parameters;

			std::optional<String> get_parameter(const String& str) const {
				for (const auto& elem : parameters) {
					if (elem.first == str) {
						return elem.second;
					}
				}
				return std::nullopt;
			}

			std::optional<long long> get_parameter_int(const String& str) const {
				const auto& val = get_parameter(str);
				if (val.has_value()) {
					return std::stoll(val.value());
				}
				return std::nullopt;
			}

			std::optional<double> get_parameter_float(const String& str) const {
				const auto& val = get_parameter(str);
				if (val.has_value()) {
					return std::stod(val.value());
				}
				return std::nullopt;
			}
			std::optional<std::vector<String>> get_parameter_list(const String& str) const {
				std::vector<String> ret{};
				for (const auto& elem : parameters) {
					if (elem.first == str) {
						ret.push_back(elem.second);
					}
				}
				if (ret.size() == 0) {
					return std::nullopt;
				}
				return ret;
			}

			std::vector<std::pair<String, String>>::iterator begin() {
				return parameters.begin();
			}

			std::vector<std::pair<String, String>>::const_iterator begin() const {
				return parameters.begin();
			}

			std::vector<std::pair<String, String>>::iterator end() {
				return parameters.end();
			}

			std::vector<std::pair<String, String>>::const_iterator end() const {
				return parameters.end();
			}

			std::vector<std::pair<String, String>>::size_type size() {
				return parameters.size();
			}
		};

		std::map<String, Section> sections;
		std::list<String> errors;
		std::shared_ptr<Format<CharT>> format;

		_Ini() : format(std::make_shared<Format<CharT>>()) {};
		_Ini(std::shared_ptr<Format<CharT>> fmt) : format(fmt) {};
		
		void write(std::basic_ostream<CharT>& os) const {
			for (auto const& sec : sections) {
				os << format->char_section_start << sec.first << format->char_section_end << std::endl;
				for (auto const& val : sec.second) {
					os << val.first << format->char_assign << val.second << std::endl;
				}
				os << std::endl;
			}
		}

		void parse(std::basic_istream<CharT>& is) {
			String line;
			String section;
			const std::locale loc{ "C" };
			while (std::getline(is, line)) {
				if (!line.empty()) {
					if (line.at(std::max<int>(0, line.length() - 1)) == 0x0D) {
						line.pop_back();
					}
				}
				detail::ltrim(line, loc);
				detail::rtrim(line, loc);
				const auto length = line.length();
				if (length > 0) {
					const auto pos = std::find_if(line.begin(), line.end(), [this](CharT ch) { return format->is_assign(ch); });
					const auto& front = line.front();
					if (format->is_comment(front)) {
					}
					else if (format->is_section_start(front)) {
						if (format->is_section_end(line.back()))
							section = line.substr(1, length - 2);
						else
							errors.push_back(line);
					}
					else if (pos != line.begin() && pos != line.end()) {
						String variable(line.begin(), pos);
						String value(pos + 1, line.end());
						detail::rtrim(variable, loc);
						detail::ltrim(value, loc);
						auto& sec = sections[section];
						sec.parameters.emplace_back(variable, value);
					}
					else {
						errors.push_back(line);
					}
				}
			}
		}

		void strip_trailing_comments() {
			const std::locale loc{ "C" };
			for (auto& sec : sections)
				for (auto& val : sec.second) {
					detail::rtrim2(val.second, [this](CharT ch) { return format->is_comment(ch); });
					detail::rtrim(val.second, loc);
				}
		}

		void clear() {
			sections.parameters.clear();
			errors.clear();
		}
	};

	using Ini = _Ini<char>;
}; // namespace inipp