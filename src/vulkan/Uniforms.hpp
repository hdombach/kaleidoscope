#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include "types/Camera.hpp"
#include "MappedUniform.hpp"

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
			return {
				camera.gen_rotate_mat(),
				camera.gen_raster_mat(),
				glm::vec4(camera.position, 0.0f),
				camera.get_aspect(),
				camera.fovy,
				camera.z_near,
				camera.z_far,
				camera.de_iterations,
				std::pow(10.0f, -camera.de_small_step),
			};
		}

		static constexpr const char *declaration_content() {
			return 
				"\tmat4 camera_rotation;\n"
				"\tmat4 camera_transformation;\n"
				"\tvec4 camera_translation;\n"
				"\tfloat aspect;\n"
				"\tfloat fovy;\n"
				"\tfloat z_near;\n"
				"\tfloat z_far;\n"
				"\tint de_iterations;\n"
				"\tfloat de_small_step;\n";
		};
	};

	struct OverlayUniform {
		alignas(4) uint32_t selected_node;

		static constexpr const char *declaration_content() {
			return
				"\tuint selected_node;\n";
		}
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

		static constexpr const char *declaration_content() {
			return
				"\tmat4 rotation;\n"
				"\tvec4 translation;\n"
				"\tfloat aspect;\n"
				"\tfloat fovy;\n"
				"\tfloat z_near;\n"
				"\tfloat z_far;\n"
				"\tuvec4 seed;\n"
				"\tuint ray_count;\n"
				"\tuint compute_index;\n"
				"\tint de_iterations;\n"
				"\tfloat de_small_step;\n";
		}
	};

	using MappedPrevPassUniform = MappedUniform<GlobalPrevPassUniform>;
	using MappedComputeUniform = MappedUniform<ComputeUniform>;
	using MappedOverlayUniform = MappedUniform<OverlayUniform>;
}
