#pragma once

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <memory>
#include <array>
#include <glm/fwd.hpp>

#include "format.hpp"

namespace util {
	template<typename Contains, typename Element>
	bool contains(Contains const &container, Element const &element) {
		return std::find(container.begin(), container.end(), element) != container.end();
	}

	inline bool replace_substr(
			std::string &str,
			std::string const substr,
			std::string const replace)
	{
		auto start = str.find(substr);
		if (start == std::string::npos) return false;
		str.erase(start, substr.size());
		str.insert(start, replace);
		return true;
	}

	template<typename Base, typename Derived>
		std::unique_ptr<Base> cast(std::unique_ptr<Derived> &&ptr) {
			return std::unique_ptr<Base>(static_cast<Base *>(ptr.release()));
		}

	inline std::string add_strnum(std::string const &str) {
		std::string result = str;
		size_t i = 0;
		size_t line = 2;
		result.insert(0, "1\t");
		while (result[i]) {
			if (result[i] == '\n') {
				result.insert(i+1, f(line, "\t"));
				line++;
			}
			i++;
		}
		return result;
	}

	inline void indent(std::string &str, std::string const &indent_str) {
		size_t i = 0;
		str.insert(0, indent_str);
		while (str[i]) {
			if (str[i] == '\n') {
				str.insert(i+1, indent_str);
			}
			i++;
		}
	}

	inline std::string indented(std::string const &str, std::string const &indent_str) {
		std::string res = str;
		indent(res, indent_str);
		return res;
	}

	inline std::array<float, 3> as_array(glm::vec3 v) {
		return {v.x, v.y, v.z};
	}
	inline glm::vec3 as_vec(std::array<float, 3> a) {
		return {a[0], a[1], a[2]};
	}

	inline std::string escape_str(std::string const &str) {
		auto res = std::string();
		for (auto c : str) {
			switch (c) {
				case '\x03':
					res += "^C";
					break;
				case '\a':
					res += "\\a";
					break;
				case '\b':
					res += "\\b";
					break;
				case '\t':
					res += "\\t";
					break;
				case '\n':
					res += "\\n";
					break;
				case '\v':
					res += "\\v";
					break;
				case '\f':
					res += "\\f";
					break;
				case '\r':
					res += "\\r";
					break;
				case '"':
					res += "\\\"";
					break;
				case '\\':
					res += "\\\\";
					break;
				case 127:
					res += "0x7f";
					break;
				default:
					res += c;
			}
		}
		return res;
	}

	inline std::string get_str_line(std::string const &str) {
		auto res = std::string();
		for (auto c : str) {
			if (c == '\n') break;
			res += c;
		}
		return res;
	}

	inline std::string trim(std::string const &str) {
		if (str.empty()) return str;
		auto begin = str.data();
		auto end = str.data()+str.size()-1;
		while (end >= str.data() && std::isspace(*end)) {
			end--;
		}
		while (*begin && std::isspace(*end)) {
			begin++;
		}
		return {begin, end+1};
	}

	template<typename T>
		inline std::string trim(T const &t) {
			auto ss = std::stringstream();
			ss << t;
			return trim(ss.str());
		}

	/**
	 * @brief Tests whether a ptr exists
	 */
	struct ptr_exists {
		template<typename T>
		bool operator()(T const *ptr) {
			return ptr;
		}
	};

	/**
	 * @brief Tests the element with a bool operator overload
	 */
	struct has_value {
		template<typename T>
		bool operator()(T const &v) {
			return static_cast<bool>(v);
		}
	};
}
