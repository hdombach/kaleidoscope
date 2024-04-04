#pragma once

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
