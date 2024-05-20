#pragma once

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
			void rotate_drag(glm::vec2 delta) {
				rotate_drag(delta.x, delta.y);
			}
	};
}
