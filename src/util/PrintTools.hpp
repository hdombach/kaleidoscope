#pragma once

#include <ostream>
#include <vector>

#include "RectSize.hpp"

namespace util {
	void print_table(std::ostream &os, std::vector<std::vector<std::string>> const &table);
	RectSize str_rect(std::string const &str);
}
