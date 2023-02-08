#include "util.h"
#include <stdexcept>
#include <string>
#include <fstream>

namespace util {
	std::string readFile(const std::string& filename) {
		std::ifstream file(filename);
		std::string result;

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}

		file.seekg(0, std::ios::end);
		result.reserve(file.tellg());
		file.seekg(0, std::ios::beg);

		result.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

		file.close();

		return result;
	}
}
