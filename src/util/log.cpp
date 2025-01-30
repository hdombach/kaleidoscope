#include <ostream>
#include <string>

#include "log.hpp"

namespace util {
	std::ostream& log(Importance importance, util::FileLocation location) {
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

		std::cout << location << "] ";

		return std::cout;
	}

}

void log_assert(
	bool test,
	std::string const &desc,
	util::FileLocation loc
) {
	if (!test) {
		log(util::Importance::FATAL_ERROR, loc) << desc << std::endl;
	}
}
