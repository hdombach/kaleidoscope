#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include "MappedUniform.hpp"

namespace vulkan {
	struct GlobalPrevPassUniform {
		alignas(16) glm::mat4 camera_transformation;
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

		static constexpr const char *declaration() {
			return
				"struct ComputeUniform {\n"
				"\tmat4 rotation;\n"
				"\tvec4 translation;\n"
				"\tfloat aspect;\n"
				"\tfloat fovy;\n"
				"\tuvec4 seed;\n"
				"\tuint ray_count;\n"
				"\tuint compute_index;\n"
				"};\n";
		}
	};

	using MappedPrevPassUniform = MappedUniform<GlobalPrevPassUniform>;
	using MappedComputeUniform = MappedUniform<ComputeUniform>;
	using MappedOverlayUniform = MappedUniform<OverlayUniform>;
}
