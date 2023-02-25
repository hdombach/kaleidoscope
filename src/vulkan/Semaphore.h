#pragma once

#include "Device.h"
#include <memory>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Semaphore;
	using SharedSemaphore = std::shared_ptr<Semaphore>;

	struct SemaphoreData {
		VkSemaphore semaphore_;
		SharedDevice device_;
	};
	struct SemaphoreDeleter {
		void operator()(SemaphoreData *) const;
	};

	class Semaphore: public std::unique_ptr<SemaphoreData, SemaphoreDeleter> {
		public:
			using base_type = std::unique_ptr<SemaphoreData, SemaphoreDeleter>;

			Semaphore() = default;
			Semaphore(SharedDevice device);
			VkSemaphore& raw();
	};

}

