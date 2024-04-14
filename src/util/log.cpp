#include "log.hpp"
#include <ostream>
#include <string>

namespace util {
	std::ostream& log(Importance importance, std::string file, int line) {
		std::cout << "[";

		if (importance & EVENT) {
			std::cout << "EVENT ";
		} else if (importance & WARNING) {
			std::cout << color::YELLOW << "WARNING ";
		} else if (importance & ERROR) {
			std::cout << color::RED << "ERROR ";
		} else if (importance & FATAL_ERROR) {
			std::cout << color::MAGENTA << "FATAL ERROR ";
		} else if (importance & MEMORY) {
			std::cout << color::CYAN << "MEMORY ";
		} else if (importance & DEBUG) {
			std::cout << color::BLUE << "DEBUG ";
		}
		std::cout << color::RESET;

		std::cout << file << ":" << line << "] ";

		return std::cout;
	}
}
