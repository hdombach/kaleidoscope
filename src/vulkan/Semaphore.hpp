#pragma once

#include <vulkan/vulkan_core.h>

#include "../util/result.hpp"

namespace vulkan {
	class Semaphore final {
		public:

			static util::Result<Semaphore, VkResult> create();
			static util::Result<Semaphore, VkResult> create(VkDevice device);

			Semaphore();

			Semaphore(const Semaphore& other) = delete;
			Semaphore(Semaphore &&other);
			Semaphore& operator=(const Semaphore& other) = delete;
			Semaphore& operator=(Semaphore&& other);

			~Semaphore();

			VkSemaphore& operator*();

			VkSemaphore const& operator*() const;

		private:
			Semaphore(VkSemaphore semaphore);

			VkSemaphore _semaphore;
	};
}
