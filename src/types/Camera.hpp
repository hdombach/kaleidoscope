#pragma once

#include <array>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/trigonometric.hpp>

#include "../util/math.hpp"

namespace types {
	class Camera {
		public:
			glm::vec3 position;
			glm::quat rotation;
			int width;
			int height;
			float fovy;
			float z_near;
			float z_far;

		public:
			Camera();

			glm::mat4 gen_raster_mat();
			glm::mat4 gen_rotate_mat();
			glm::mat4 gen_translate_mat();

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

			inline std::array<float, 3> get_euler_rotation() {
				auto q = util::quaternion_to_euler(rotation);
				return std::array<float, 3>{
					glm::degrees(q.x),
					glm::degrees(q.y),
					glm::degrees(q.z)
				};
			};
			inline void set_euler_rotation(std::array<float, 3> &r) {
				this->rotation = util::euler_to_quaterniion({
						glm::radians(r[0]),
						glm::radians(r[1]),
						glm::radians(r[2])
				});
			}
	};
}
