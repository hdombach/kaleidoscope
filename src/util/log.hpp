#pragma once

#include <_types/_uint8_t.h>
#include <ostream>
#include <iostream>

#define DEFAULT_LOG_EVENT true
#define DEFAULT_LOG_WARNING true
#define DEFAULT_LOG_ERROR true
#define DEFAULT_LOG_FATAL_ERROR true
#define DEFAULT_LOG_MEMORY true

namespace util {
	enum Importance: uint8_t {
		EVENT        = 0b10000000,
		WARNING      = 0b01000000,
		ERROR        = 0b00100000,
		FATAL_ERROR  = 0b00010000,
		MEMORY       = 0b00001000,
		DEBUG        = 0b00000100,
		ALL          = 0b11111100,
		NONE         = 0b00000000,
	};

	extern Importance g_log_flags;

	std::ostream& log(Importance importance, std::string file, int line);

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
}

#define LOG(importance) log(importance, __FILE__, __LINE__)
#define LOG_EVENT log(util::Importance::EVENT, __FILE__, __LINE__)
#define LOG_WARNING log(util::Importance::WARNING, __FILE__, __LINE__)
#define LOG_ERROR log(util::Importance::ERROR, __FILE__, __LINE__)
#define LOG_FATAL_ERROR log(util::Importance::FATAL_ERROR, __FILE__, __LINE__)
#define LOG_MEMORY log(util::Importance::MEMORY, __FILE__, __LINE__)
#define LOG_DEBUG log(util::Importance::DEBUG, __FILE__, __LINE__)
