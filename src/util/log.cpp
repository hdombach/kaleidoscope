#include "log.hpp"
#include <ostream>
#include <source_location>
#include <string>

namespace util {
	std::ostream& log(Importance importance, std::source_location location) {
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

		std::cout << location.file_name() << "(" << location.line() << ":" << location.column() << ")" << "] ";

		return std::cout;
	}

}

void log_assert(
	bool test,
	std::string const &desc,
	std::source_location loc
) {
	if (!test) {
		log(util::Importance::FATAL_ERROR, loc) << desc << std::endl;
	}
}
