#pragma once

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "vertex.hpp"
#include <vulkan/vulkan.h>

namespace vulkan {
	class Mesh {
		public:
			virtual ~Mesh() = default;
			virtual VkBuffer vertexBuffer() const = 0;
			virtual VkBuffer indexBuffer() const = 0;
			virtual uint32_t indexCount() const = 0;
	};
}
