#pragma once

#include <ostream>
#include <vector>

#include "RectSize.hpp"

namespace util {
	void print_table(std::ostream &os, std::vector<std::vector<std::string>> const &table);
	RectSize str_rect(std::string const &str);

	template<typename T>
	void print_list(std::ostream &os, std::vector<T> &l) {
		os << "[";
		for (uint32_t i = 0; i < l.size(); i++) {
			if (i) os << ", ";
			os << l[i];
		}
		os << "]" << std::endl;
	}
}
