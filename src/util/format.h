#pragma once

#include <sstream>
namespace util {
	inline void fImpl_(std::stringstream &) {};
	template<typename T, typename... Types>
		inline void fImpl_(std::stringstream &ss, T arg1, Types... args) {
			ss << arg1;
			fImpl_(ss, args...);
		}

	template<typename... Types>
		inline std::string f(Types... args) {
			std::stringstream ss;
			fImpl_(ss, args...);
			return ss.str();
		}
}
