#include "Camera.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace types {
	Camera::Camera() {
		position = glm::vec3(0, -1, 0);
		rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		width = 100;
		height = 100;
		fovy = 45;
		z_near = 0.1f;
		z_far = 10.0f;
	}

	glm::mat4 Camera::gen_mat() {
		auto result = glm::perspective(
				glm::radians(fovy), 
				width / (float) height, 
				z_near, 
				z_far);
		result *= glm::mat4_cast(rotation);
		result = glm::translate(result, position);
		return result;
	}

	void Camera::rotate_drag(float deltax, float deltay) {
		auto rotation_start = glm::vec3(0, 0, 1);
	}
}
