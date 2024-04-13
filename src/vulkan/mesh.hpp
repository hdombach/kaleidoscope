#pragma once

#include <vulkan/vulkan.h>

namespace vulkan {
	class Mesh {
		public:
			virtual ~Mesh() = default;
			virtual VkBuffer vertex_buffer() const = 0;
			virtual VkBuffer index_buffer() const = 0;
			virtual uint32_t index_count() const = 0;
	};
}
