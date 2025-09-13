#include "math.hpp"
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include "glm/gtx/quaternion.hpp"

namespace util {

	//https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	glm::vec3 quaternion_to_euler_old(glm::quat q) {
		auto result = glm::vec3();

		// roll (x-axis rotation)
		double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
		double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
		result.x = std::atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		double sinp = std::sqrt(1 + 2 * (q.w * q.y - q.x * q.z));
		double cosp = std::sqrt(1 - 2 * (q.w * q.y - q.x * q.z));
		result.y = 2 * std::atan2(sinp, cosp) - M_PI / 2;

		// yaw (z-axis rotation)
		double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
		double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
		result.z = std::atan2(siny_cosp, cosy_cosp);

		return result;
	}

	glm::vec3 quaternion_to_euler(glm::quat q) {
		auto v = glm::vec3();

		auto m = glm::mat3_cast(q);

		//zxy ordering from https://github.com/mrdoob/three.js/blob/dev/src/math/Euler.js
		v.x = asin(-glm::clamp(m[2][1], -1.0f, 1.0f));

		if (m[2][1] < 0.999999) {
			v.y = atan2(m[2][0], m[2][2]);
			v.z = atan2(m[0][1], m[1][1]);
		} else {
			v.y = 0;
			v.z = atan2(m[1][0], m[0][0]);
		}

		return v;
	}


	glm::quat euler_to_quaterniion(glm::vec3 e) {
		double cr = cos(e.x * 0.5);
		double sr = sin(e.x * 0.5);
		double cp = cos(e.y * 0.5);
		double sp = sin(e.y * 0.5);
		double cy = cos(e.z * 0.5);
		double sy = sin(e.z * 0.5);

		auto result = glm::quat();
		result.w = cr * cp * cy + sr * sp * sy;
		result.x = sr * cp * cy - cr * sp * sy;
		result.y = cr * sp * cy + sr * cp * sy;
		result.z = cr * cp * sy - sr * sp * cy;

		return result;
	}
}
