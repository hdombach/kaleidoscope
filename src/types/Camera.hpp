#pragma once

#include <array>
#include <memory>

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/trigonometric.hpp>

#include "types/Node.hpp"

namespace types {
	class Camera: public vulkan::Node {
		public:
			using Ptr = std::unique_ptr<Camera>;

			static Ptr create(uint32_t id);
			static Ptr create_virtual(uint32_t id) = delete;

			bool operator==(const Camera &other) const;
			bool operator!=(const Camera &other) const;

			int width() const;
			void set_width(int w);
			int height() const;
			void set_height(int h);
			float fovy() const;
			void set_fovy(float fovy);
			float z_near() const;
			void set_z_near(float z_near);
			float z_far() const;;
			void set_z_far(float z_far);
			int de_iterations() const;
			void set_de_iterations(int de_iterations);
			float de_small_step() const;
			void set_de_small_step(float de_small_step);

			glm::mat4 gen_raster_mat() const;
			float get_aspect() const;

			void rotate_drag(float deltax, float deltay);
			void rotate_drag(glm::vec2 delta);

			using Node::set_position;
			std::array<float, 3> get_position_array();
			void set_position(std::array<float, 3> &position);

			using Node::set_rotation;
			std::array<float, 3> get_rotation_array();
			void set_rotation(std::array<float, 3> &r);

		private:
			glm::quat _q;
			int _width;
			int _height;
			float _fovy;
			float _z_near;
			float _z_far;
			int _de_iterations;
			float _de_small_step;

		private:
			Camera(uint32_t id, Type type): Node(id, type) {}
	};
}
