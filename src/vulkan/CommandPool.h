#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "Device.h"

namespace vulkan {
	class CommandPool;
	using SharedCommandPool = std::shared_ptr<CommandPool>;

	struct CommandPoolData {
		VkCommandPool commandPool_;
		SharedDevice device_;
	};
	struct CommandPoolDeleter {
		void operator()(CommandPoolData *data) const;
	};

	class CommandPool: public std::unique_ptr<CommandPoolData, CommandPoolDeleter> {
		public:
			using base_type = std::unique_ptr<CommandPoolData, CommandPoolDeleter>;

			CommandPool(SharedDevice device, PhysicalDevice physicalDevice);
			VkCommandPool& raw();
	};
}
