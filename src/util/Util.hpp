#pragma once

#include <algorithm>

namespace util {
	template<typename Contains, typename Element>
	bool contains(Contains &container, Element &element) {
		return std::find(container.begin(), container.end(), element) != container.end();
	}
}
