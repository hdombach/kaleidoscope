#pragma once

#include <sstream>
namespace util {
	inline void stringConcatImpl_(std::stringstream &) {};
	template<typename T, typename... Types>
		inline void stringConcatImpl_(std::stringstream &ss, T arg1, Types... args) {
			ss << arg1;
			stringConcatImpl_(ss, args...);
		}

	template<typename... Types>
		inline std::string stringConcat(Types... args) {
			std::stringstream ss;
			stringConcatImpl_(ss, args...);
			return ss.str();
		}
}
