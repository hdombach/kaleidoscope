#include <ostream>
#include <string>

#include "log.hpp"
#include "NullStream.hpp"

namespace util {
	uint8_t g_log_flags = DEBUG | WARNING | ERROR | FATAL_ERROR;

	std::ostream& log(
		Importance importance,
		util::FileLocation location,
		std::string const &tag_postfix
	) {
		if (!(importance & g_log_flags)) {
			return util::null_stream;
		}

		std::cout << "[";

		if (importance & INFO) {
			std::cout << "EVENT";
		} else if (importance & WARNING) {
			std::cout << color::YELLOW << "WARNING";
		} else if (importance & ERROR) {
			std::cout << color::RED << "ERROR";
		} else if (importance & FATAL_ERROR) {
			std::cout << color::MAGENTA << "FATAL ERROR";
		} else if (importance & MEMORY) {
			std::cout << color::CYAN << "MEMORY";
		} else if (importance & DEBUG) {
			std::cout << color::BLUE << "DEBUG";
		} else if (importance & TRACE) {
			std::cout << color::BLUE << "TRACE";
		}
		std::cout << tag_postfix;
		std::cout << color::RESET;

		std::cout << " " << location << "] ";

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

std::ostream &_log_every_n(
	uint32_t &_counter,
	Importance importance,
	uint32_t n,
	util::FileLocation loc,
	std::string const &tag_postfix
) {
	uint32_t orig = _counter;
	_counter = (_counter + 1) % n;
	if (orig == 0) {
		return log(importance, loc, tag_postfix);
	} else {
		return util::null_stream;
	}
}

std::ostream &_log_every_t(
	util::TimePoint &time,
	Importance importance,
	std::chrono::milliseconds t,
	util::FileLocation loc,
	std::string const &tag_postfix
) {
	auto now = std::chrono::steady_clock::now();
	if (t < (now.time_since_epoch() - time.value.time_since_epoch())) {
		time.value = now;
		return log(importance, loc, tag_postfix);
	} else {
		return util::null_stream;
	}
}
