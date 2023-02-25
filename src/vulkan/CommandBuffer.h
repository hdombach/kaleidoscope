#pragma once

#include "CommandPool.h"
#include "Device.h"
#include <memory>

namespace vulkan {
	class CommandBuffer;
	using SharedCommandBuffer = std::shared_ptr<CommandBuffer>;

	struct CommandBufferData {
		VkCommandBuffer commnadBuffer_;
		SharedDevice device_;
	};

	class CommandBuffer: public std::unique_ptr<CommandBufferData> {
		public:
			using base_type = std::unique_ptr<CommandBufferData>;

			CommandBuffer(SharedDevice device, SharedCommandPool commandPool);
			VkCommandBuffer &raw();
	};
}
