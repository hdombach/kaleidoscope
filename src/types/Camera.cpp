#include "Camera.hpp"
#include <cmath>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include "util/math.hpp"

namespace types {
	Camera::Ptr Camera::create(uint32_t id) {
		auto result = Ptr(new Camera(id, Node::Type::Camera));

		result->set_position({0, -1, 0});
		result->_q = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		result->set_rotation(util::quaternion_to_euler(result->_q));
		result->_width = 500;
		result->_height = 500;
		result->_fovy = 45;
		result->_z_near = 0.01f;
		result->_z_far = 10.0f;
		result->_de_iterations = 120;
		result->_de_small_step = 3;

		result->name() = "Camera " + std::to_string(id);

		return std::move(result);
	}

	bool Camera::operator==(const Camera &other) const {
		return id() == other.id();
	}

	bool Camera::operator!=(const Camera &other) const {
		return !(*this == other);
	}

	int Camera::width() const {
		return _width;
	}

	void Camera::set_width(int w) {
		_width = w;
	}

	int Camera::height() const {
		return _height;
	}

	void Camera::set_height(int h) {
		_height = h;
	}

	float Camera::fovy() const {
		return _fovy;
	}

	void Camera::set_fovy(float fovy) {
		_fovy = fovy;
	}

	float Camera::z_near() const {
		return _z_near;
	}

	void Camera::set_z_near(float z_near) {
		_z_near = z_near;
	}

	float Camera::z_far() const {
		return _z_far;
	}

	void Camera::set_z_far(float z_far) {
		_z_far = z_far;
	}

	int Camera::de_iterations() const {
		return _de_iterations;
	}

	void Camera::set_de_iterations(int de_iterations) {
		_de_iterations = de_iterations;
	}

	float Camera::de_small_step() const {
		return _de_small_step;
	}

	void Camera::set_de_small_step(float small_step) {
		_de_small_step = small_step;
	}

	glm::mat4 Camera::gen_raster_mat() const {
		auto result = glm::mat4(1.0);
		auto perspective = glm::perspective(
				glm::radians(_fovy), 
				(float) _width / (float) _height, 
				_z_near, 
				_z_far);
		perspective[1][1] *= -1;
		result *= perspective;

		auto p = get_matrix() * glm::vec4(0, 0, 0, 1);

		result *= glm::inverse(rotation_matrix());
		result = glm::translate(result, glm::vec3(-p.x, -p.y, -p.z));

		return result;
	}

	float Camera::get_aspect() const {
		return static_cast<float>(_width) / static_cast<float>(_height);
	}

	void Camera::rotate_drag(float deltax, float deltay) {
		auto rotation_start = glm::vec3(0, 0, 1);
		auto rotation_end = glm::vec3(deltax, deltay, 1);
		rotation_end = glm::normalize(rotation_end);

		auto angle = acos(glm::dot(rotation_start, rotation_end));
		auto r = glm::normalize(glm::cross(rotation_start, rotation_end));
		auto rotation_axis = glm::vec4(r.x, r.y, r.z, 0);
		rotation_axis = rotation_matrix() * rotation_axis;

		if (!std::isnan(rotation_axis.x)) {
			auto rotation_quat = glm::angleAxis(angle, glm::vec3(rotation_axis.x, rotation_axis.y, rotation_axis.z));
			this->_q = glm::normalize(this->_q * rotation_quat);
			set_rotation(util::quaternion_to_euler(this->_q));
		}
	}

	void Camera::rotate_drag(glm::vec2 delta) {
		rotate_drag(delta.x, -delta.y);
	}

	std::array<float, 3> Camera::get_position_array() {
		return std::array<float, 3>{position().x, position().y, position().z};
	}

	void Camera::set_position(std::array<float, 3> &position) {
		Node::set_position(glm::vec3(position[0], position[1], position[2]));
	}

	std::array<float, 3> Camera::get_rotation_array() {
		return std::array<float, 3>{rotation().x, rotation().y, rotation().z};
	}

	void Camera::set_rotation(std::array<float, 3> &r) {
		Node::set_rotation(glm::vec3(r[0], r[1], r[2]));
	}
}
