#pragma once

#include <algorithm>
#include <string>

namespace util {
	template<typename Contains, typename Element>
	bool contains(Contains &container, Element &element) {
		return std::find(container.begin(), container.end(), element) != container.end();
	}

	inline void replace_substr(
			std::string &str,
			std::string const &substr,
			std::string const &replace)
	{
		auto start = str.find(substr);
		str.erase(start, substr.size());
		str.insert(start, replace);
	}
}
