#pragma once

#include "errors.h"
#include "result.h"
#include <vulkan/vulkan.h>
#include "vertex.h"

namespace vulkan {
	class Mesh {
		public:
			virtual ~Mesh() = default;
			virtual VkBuffer vertexBuffer() const = 0;
			virtual VkBuffer indexBuffer() const = 0;
			virtual uint32_t indexCount() const = 0;
	};
}
