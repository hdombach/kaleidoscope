#pragma once

#include <array>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace types {
	class Camera {
		public:
			Camera();

			glm::mat4 gen_mat();

			glm::vec3 position;
			glm::quat rotation;
			int width;
			int height;
			float fovy;
			float z_near;
			float z_far;

			void rotate_drag(float deltax, float deltay);
			inline void rotate_drag(glm::vec2 delta) {
				rotate_drag(delta.x, -delta.y);
			}

			inline std::array<float, 3> get_position_array() {
				return std::array<float, 3>{position.x, position.y, position.z};
			}
			inline void set_position(std::array<float, 3> &position) {
				this->position.x = position[0];
				this->position.y = position[1];
				this->position.z = position[2];
			}
	};
}
