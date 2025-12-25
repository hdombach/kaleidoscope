#pragma once

#include <chrono>
#include <ostream>
#include <iostream>
#include <source_location>

#include "FileLocation.hpp"

#define DEFAULT_LOG_INFO true
#define DEFAULT_LOG_WARNING true
#define DEFAULT_LOG_ERROR true
#define DEFAULT_LOG_FATAL_ERROR true
#define DEFAULT_LOG_MEMORY true

namespace util {
	enum Importance: uint8_t {
		INFO        = 0b10000000,
		WARNING      = 0b01000000,
		ERROR        = 0b00100000,
		FATAL_ERROR  = 0b00010000,
		MEMORY       = 0b00001000,
		DEBUG        = 0b00000100,
		TRACE        = 0b00000010,
		ALL          = 0b11111110,
		NONE         = 0b00000000,
	};

	namespace color {
		static const char *RESET    = "\033[0m";
		static const char *BLACK    = "\033[30m";
		static const char *RED      = "\033[31m";
		static const char *GREEN    = "\033[32m";
		static const char *YELLOW   = "\033[33m";
		static const char *BLUE     = "\033[34m";
		static const char *MAGENTA  = "\033[35m";
		static const char *CYAN     = "\033[36m";
		static const char *WHILE    = "\033[37m";
	}

	extern uint8_t g_log_flags;

	std::ostream& log(
		Importance importance,
		util::FileLocation loc=std::source_location::current(),
		std::string const &tag_postfix=""
	);

	struct TimePoint {
		std::chrono::time_point<std::chrono::steady_clock> value;
	};
}

void log_assert(
	bool test,
	std::string const &desc,
	util::FileLocation=std::source_location::current()
);

/**
 * @brief General information that appears in verbose mode
 */
inline std::ostream& log_info(
	util::FileLocation loc=std::source_location::current(),
	std::string const &tag_postfix=""
) {
	return log(util::Importance::INFO, loc);
}
/**
 * @brief Information about potentially incorrect state
 */
inline std::ostream& log_warning(
	util::FileLocation loc=std::source_location::current(),
	std::string const &tag_postfix=""
) {
	return log(util::Importance::WARNING, loc);
}
/**
 * @brief When there is an error
 */
inline std::ostream& log_error(
	util::FileLocation loc=std::source_location::current(),
	std::string const &tag_postfix=""
) {
	return log(util::Importance::ERROR, loc);
}
/**
 * @brief An error that cannot be recovered from
 */
inline std::ostream& log_fatal_error(
	util::FileLocation loc=std::source_location::current(),
	std::string const &tag_postfix=""
) {
	return log(util::Importance::FATAL_ERROR, loc);
}
/**
 * @brief Used for debugging memory related issues
 */
inline std::ostream& log_memory(
	util::FileLocation loc=std::source_location::current(),
	std::string const &tag_postfix=""
) {
	return log(util::Importance::MEMORY, loc);
}
/**
 * @brief Only used for debugging. Should be removed when problem is fixed.
 */
inline std::ostream& log_debug(
	util::FileLocation loc=std::source_location::current(),
	std::string const &tag_postfix=""
) {
	return log(util::Importance::DEBUG, loc);
}
/**
 * @brief The most verbse level of logging.
 */
inline std::ostream& log_trace(
	util::FileLocation loc=std::source_location::current(),
	std::string const &tag_postfix=""
) {
	return log(util::Importance::TRACE, loc, tag_postfix);
}
/**
 * @brief Start a timer that can be used when logging
 */
inline util::TimePoint log_start_timer() {
	return {std::chrono::steady_clock::now()};
}

inline std::ostream& operator<<(std::ostream& os, util::TimePoint const &start) {
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start.value);
	os << elapsed.count() << "ms";
	return os;
}
