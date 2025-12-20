#include <ostream>
#include <string>

#include "log.hpp"
#include "NullStream.hpp"

namespace util {
	uint8_t g_log_flags = DEBUG | WARNING | ERROR | FATAL_ERROR;

	std::ostream& log(Importance importance, util::FileLocation location) {
		if (!(importance & g_log_flags)) {
			return util::null_stream;
		}

		std::cout << "[";

		if (importance & INFO) {
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
		} else if (importance & TRACE) {
			std::cout << color::BLUE << "TRACE ";
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
