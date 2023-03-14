#include "log.h"
#include <bitset>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace util {
	Log::Log(std::ostream &os, ImportanceMask::type importance_flags):
		os_(os),
		remembered_logs_(importance_flags)
	{
		std::stringstream ss;
		std::bitset<8> bits = importance_flags;
		ss << "importance: " << bits;
		std::cout << ss.str() << std::endl;
		util::log_event(ss.str());
	}

	Log::Log(std::string name, ImportanceMask::type importance_flags, std::ostream &os):
		name_(name),
		remembered_logs_(importance_flags),
		os_(os)
	{}

	void Log::remember(Importance::Importance_t importance) {
		remembered_logs_ |= importance;
	}

	void Log::ignore(Importance::Importance_t importance) {
		remembered_logs_ &= !importance;
	}

	void Log::log(Importance::Importance_t importance, std::string message) {
		if (importance == Importance::EVENT) {
			log_event(message);
		} else if (importance == Importance::WARNING) {
			log_warning(message);
		} else if (importance == Importance::ERROR) {
			log_error(message);
		} else if (importance == Importance::FATAL_ERROR) {
			log_fatal_error(message);
		} else if (importance == Importance::MEMORY) {
			log_memory(message);
		} else {
			std::stringstream ss;
			ss << "Invalid event importance: " << importance;
			log_fatal_error(ss.str());
		}
	}

	void Log::log_event(std::string message) {
		if (remembered_logs_ & ImportanceMask::EVENT) {
			log_impl_("EVENT", message);
		}
	}
	void Log::log_warning(std::string message) {
		if (remembered_logs_ & ImportanceMask::WARNING) {
			log_impl_("WARNING", message);
		}
	}
	void Log::log_error(std::string message) {
		if (remembered_logs_ & ImportanceMask::ERROR) {
			log_impl_("ERROR", message);
		}
	}
	void Log::log_fatal_error(std::string message) {
		if (remembered_logs_ & ImportanceMask::FATAL_ERROR) {
			log_impl_("FATAL_ERROR", message);
			throw std::runtime_error("Received fatal error");
		}
	}
	void Log::log_memory(std::string message) {
		if (remembered_logs_ & ImportanceMask::MEMORY) {
			log_impl_("MEMORY", message);
		}
	}

	void Log::log_impl_(std::string importance_repr, std::string message) {
		if (!name_.empty()) {
			os_ << name_ << " ";
		}
		os_ << importance_repr << ": " << message << "\n";
	}


	void log(Importance::Importance_t importance, std::string message) {
		default_log.log(importance, message);
	}
	void log_event(std::string message) {
		default_log.log_event(message);
	}
	void log_warning(std::string message) {
		default_log.log_warning(message);
	}
	void log_error(std::string message) {
		default_log.log_error(message);
	}
	void log_fatal_error(std::string message) {
		default_log.log_fatal_error(message);
	}
	void log_memory(std::string message) {
		default_log.log_memory(message);
	}
}
