#pragma once

#include <algorithm>
#include <string>
#include <memory>
#include <array>
#include <glm/fwd.hpp>

#include "format.hpp"

namespace util {
	template<typename Contains, typename Element>
	bool contains(Contains &container, Element &element) {
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

	inline void add_strnum(std::string &str) {
		size_t i = 0;
		size_t line = 2;
		str.insert(0, "1\t");
		while (str[i]) {
			if (str[i] == '\n') {
				str.insert(i+1, f(line, "\t"));
				line++;
			}
			i++;
		}
	}

	inline void indent(std::string &str, std::string indent) {
		size_t i = 0;
		str.insert(0, indent);
		while (str[i]) {
			if (str[i] == '\n') {
				str.insert(i+1, indent);
			}
			i++;
		}
	}

	inline std::array<float, 3> as_array(glm::vec3 v) {
		return {v.x, v.y, v.z};
	}
	inline glm::vec3 as_vec(std::array<float, 3> a) {
		return {a[0], a[1], a[2]};
	}

	template<typename T>
	inline bool exists(std::unique_ptr<T> const &ptr) {
		return static_cast<bool>(ptr);
	}
}
