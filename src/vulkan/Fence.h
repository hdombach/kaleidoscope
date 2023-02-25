#pragma once

#include <memory>
#include <vulkan/vulkan.h>
#include "Device.h"

namespace vulkan {
	class Fence;
	using SharedFence = std::shared_ptr<Fence>;

	struct FenceData {
		VkFence fence_;
		SharedDevice device_;
	};
	struct FenceDeleter {
		void operator()(FenceData *) const;
	};

	class Fence: public std::unique_ptr<FenceData, FenceDeleter> {
		public:
			using base_type = std::unique_ptr<FenceData, FenceDeleter>;

			Fence() = default;
			Fence(SharedDevice device);
			VkFence& raw();
	};
}
