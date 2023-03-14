#pragma once

#include <_types/_uint8_t.h>
#include <ostream>
#include <iostream>

#define DEFAULT_LOG_EVENT true
#define DEFAULT_LOG_WARNING true
#define DEFAULT_LOG_ERROR true
#define DEFAULT_LOG_FATAL_ERROR true
#define DEFAULT_LOG_MEMORY false

namespace util {
	namespace ImportanceMask {
		using type = uint8_t;
		enum ImportanceMask_t: type {
			EVENT        = 0b10000000,
			WARNING      = 0b01000000,
			ERROR        = 0b00100000,
			FATAL_ERROR  = 0b00010000,
			MEMORY       = 0b00001000,
			ALL          = 0b11111000,
			NONE         = 0b00000000,
		};
	}
	namespace Importance {
		using type = uint8_t;
		enum Importance_t: type {
			EVENT       = 0,
			WARNING     = 1,
			ERROR       = 2,
			FATAL_ERROR = 3,
			MEMORY      = 4,

		};
	}

	class Log {
		public:

			Log(std::ostream &os, ImportanceMask::type importance_flags);
			Log(std::string name, ImportanceMask::type importance_flags, std::ostream &os);

			/**
			 * The types of logs to record
			 */
			void remember(Importance::Importance_t importance);
			/**
			 * The types of logs to ignore
			 */
			void ignore(Importance::Importance_t importance);

			/**
			 * Logs a error with the corresponding importance
			 * @throws runtime_error if importance is FATAL_ERROR
			 */
			void log(Importance::Importance_t importance, std::string message);
			void log_event(std::string message);
			void log_warning(std::string message);
			void log_error(std::string message);
			void log_fatal_error(std::string message);
			void log_memory(std::string message);

		private:
			std::string name_;
			std::ostream &os_;
			Importance::type remembered_logs_;

			void log_impl_(std::string importance_repr, std::string message);
	};


	static Log default_log = Log(
			std::cout,
			(DEFAULT_LOG_EVENT ? ImportanceMask::EVENT : ImportanceMask::NONE) |
			(DEFAULT_LOG_WARNING ? ImportanceMask::WARNING : ImportanceMask::NONE) |
			(DEFAULT_LOG_ERROR ? ImportanceMask::ERROR : ImportanceMask::NONE) |
			(DEFAULT_LOG_FATAL_ERROR ? ImportanceMask::FATAL_ERROR : ImportanceMask::NONE) |
			(DEFAULT_LOG_MEMORY ? ImportanceMask::MEMORY : ImportanceMask::NONE));

	void log(Importance::Importance_t importance, std::string message);
	void log_event(std::string message);
	void log_warning(std::string message);
	void log_error(std::string message);
	void log_fatal_error(std::string message);
	void log_memory(std::string message);
}
