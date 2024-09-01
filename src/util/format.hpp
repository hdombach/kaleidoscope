#pragma once

#include <glm/ext.hpp>

#include <sstream>
#include <ostream>

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

inline std::ostream& operator<<(std::ostream& os, glm::vec3 const &v) {
	return os << "[" << v.x << "," << v.y << "," << v.z << "]";
}
