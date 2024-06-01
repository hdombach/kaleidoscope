#include "Camera.hpp"
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace types {
	Camera::Camera() {
		position = glm::vec3(0, -1, 0);
		rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		width = 100;
		height = 100;
		fovy = 45;
		z_near = 0.1f;
		z_far = 10.0f;
	}

	glm::mat4 Camera::gen_raster_mat() {
		auto result = glm::mat4(1.0);
		auto perspective = glm::perspective(
				glm::radians(fovy), 
				width / (float) height, 
				z_near, 
				z_far);
		perspective[1][1] *= -1;
		result *= perspective;
		result *= gen_rotate_mat();
		result *= gen_translate_mat();
		return result;
	}

	glm::mat4 Camera::gen_rotate_mat() {
		return glm::toMat4(rotation);
	}

	glm::mat4 Camera::gen_translate_mat() {
		return glm::translate(glm::mat4(1.0), position);
	}

	void Camera::rotate_drag(float deltax, float deltay) {
		auto rotation_start = glm::vec3(0, 0, 1);
		auto rotation_end = glm::vec3(deltax, deltay, 1);
		rotation_end = glm::normalize(rotation_end);

		auto angle = acos(glm::dot(rotation_start, rotation_end));
		auto rotation_axis = glm::normalize(glm::cross(rotation_start, rotation_end));
		rotation_axis = rotation_axis * rotation;
		if (!std::isnan(rotation_axis.x)) {
			auto rotation_quat = glm::angleAxis(angle, rotation_axis);
			this->rotation = glm::normalize(this->rotation * rotation_quat);
		}
	}
}
