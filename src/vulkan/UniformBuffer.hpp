#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>

#include "MappedUniform.hpp"

namespace vulkan {
	struct GlobalUniformBuffer {
		alignas(16) glm::mat4 camera_transformation;
	};

	struct ComputeUniformBuffer {
		alignas(16) glm::mat4 camera_rotation;
		alignas(4) glm::vec4 camera_translation;
		alignas(4) float aspect;
		alignas(4) float fovy;
		alignas(4) uint32_t seed;
		alignas(4) uint32_t ray_count;
		alignas(4) uint32_t compute_index;

		static constexpr const char *declaration() {
			return
				"struct ComputeUniform {\n"
				"\tmat4 rotation;\n"
				"\tvec4 translation;\n"
				"\tfloat aspect;\n"
				"\tfloat fovy;\n"
				"\tuint seed;\n"
				"\tuint ray_count;\n"
				"\tuint compute_index;\n"
				"};\n";
		}
	};

	using MappedGlobalUniform = MappedUniform<GlobalUniformBuffer>;
	using MappedComputeUniform = MappedUniform<ComputeUniformBuffer>;
}
