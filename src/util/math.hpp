#pragma once

#include <glm/fwd.hpp>

namespace util {
	glm::vec3 quaternion_to_euler(glm::quat quaternion);
	glm::quat euler_to_quaterniion(glm::vec3 e);
}
