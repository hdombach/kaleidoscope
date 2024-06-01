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
		alignas(4) glm::vec4 camera_translation;
		alignas(16) glm::mat4 camera_rotation;
	};

	using MappedGlobalUniform = MappedUniform<GlobalUniformBuffer>;
	using MappedComputeUniform = MappedUniform<ComputeUniformBuffer>;
}
