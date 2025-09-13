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

inline std::ostream& operator<<(std::ostream& os, glm::vec4 const &v) {
	return os << "[" << v.x << "," << v.y << "," << v.z << "," << v.w << "]";
}

inline std::ostream& operator<<(std::ostream& os, glm::vec3 const &v) {
	return os << "[" << v.x << "," << v.y << "," << v.z << "]";
}

inline std::ostream& operator<<(std::ostream& os, glm::vec2 const &v) {
	return os << "[" << v.x << "," << v.y << "]";
}

inline std::ostream& operator<<(std::ostream& os, glm::mat4 const &m) {
	return os << "[" << m[0][0] << "," << m[0][1] << "," << m[0][2] << "," << m[0][3]
		<< "," << m[1][0] << "," << m[1][1] << "," << m[1][2] << "," << m[1][3]
		<< "," << m[2][0] << "," << m[2][1] << "," << m[2][2] << "," << m[2][3]
		<< "," << m[3][0] << "," << m[3][1] << "," << m[3][2] << "," << m[3][3]
		<< "]";
}
