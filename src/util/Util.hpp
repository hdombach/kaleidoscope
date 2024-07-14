#pragma once

#include <algorithm>
#include <string>
#include <memory>

namespace util {
	template<typename Contains, typename Element>
	bool contains(Contains &container, Element &element) {
		return std::find(container.begin(), container.end(), element) != container.end();
	}

	inline void replace_substr(
			std::string &str,
			std::string const substr,
			std::string const replace)
	{
		auto start = str.find(substr);
		str.erase(start, substr.size());
		str.insert(start, replace);
	}

	template<typename Base, typename Derived>
		std::unique_ptr<Base> cast(std::unique_ptr<Derived> &&ptr) {
			return std::unique_ptr<Base>(static_cast<Base *>(ptr.release()));
		}
}
