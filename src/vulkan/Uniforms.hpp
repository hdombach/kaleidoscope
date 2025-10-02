#pragma once

#include "util/log.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include "types/Camera.hpp"
#include "MappedUniform.hpp"
#include "TemplUtils.hpp"

namespace vulkan {
	struct GlobalPrevPassUniform {
		alignas(16) glm::mat4 camera_rotation;
		alignas(16) glm::mat4 camera_transformation;
		alignas(4) glm::vec4 camera_translation;
		alignas(4) float aspect;
		alignas(4) float fovy;
		alignas(4) float z_near;
		alignas(4) float z_far;
		alignas(4) int32_t de_iterations;
		alignas(4) float de_small_step;

		static GlobalPrevPassUniform create(types::Camera const &camera) {
			glm::vec3 angle;
			glm::extractEulerAngleYXZ(glm::inverse(camera.rotation_matrix()), angle.y, angle.x, angle.z);
			return {
				camera.rotation_matrix(),
				camera.gen_raster_mat(),
				camera.get_matrix() * glm::vec4(0, 0, 0, 1),
				camera.get_aspect(),
				camera.fovy(),
				camera.z_near(),
				camera.z_far(),
				camera.de_iterations(),
				std::pow(10.0f, -camera.de_small_step()),
			};
		}

		inline const static auto declaration_content = std::vector{
				templ_property("mat4", "camera_rotation"),
				templ_property("mat4", "camera_transformation"),
				templ_property("vec4", "camera_translation"),
				templ_property("float", "aspect"),
				templ_property("float", "fovy"),
				templ_property("float", "z_near"),
				templ_property("float", "z_far"),
				templ_property("int", "de_iterations"),
				templ_property("float", "de_small_step")
			};
	};

	struct OverlayUniform {
		alignas(4) uint32_t selected_node;

		inline const static auto declaration_content = std::vector{
			templ_property("uint", "selected_node")
		};
	};

	struct ComputeUniform {
		alignas(16) glm::mat4 camera_rotation;
		alignas(4) glm::vec4 camera_translation;
		alignas(4) float aspect;
		alignas(4) float fovy;
		alignas(16) glm::u32vec4 seed;
		alignas(4) uint32_t ray_count;
		alignas(4) uint32_t compute_index;
		alignas(4) int32_t de_iterations;
		alignas(4) float de_small_step;

		inline const static auto declarations = std::vector{
			templ_property("mat4", "camera_rotation"),
			templ_property("vec4", "camera_translation"),
			templ_property("float", "aspect"),
			templ_property("float", "fovy"),
			templ_property("float", "z_near"),
			templ_property("float", "z_far"),
			templ_property("uvec4", "seed"),
			templ_property("uint", "ray_count"),
			templ_property("uint", "compute_index"),
			templ_property("int", "de_iterations"),
			templ_property("float", "de_small_step")
		};
	};

	using MappedPrevPassUniform = MappedUniform<GlobalPrevPassUniform>;
	using MappedComputeUniform = MappedUniform<ComputeUniform>;
	using MappedOverlayUniform = MappedUniform<OverlayUniform>;
}
