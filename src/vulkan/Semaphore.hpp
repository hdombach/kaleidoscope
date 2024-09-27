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

			void destroy();
			~Semaphore() { destroy(); }

			operator bool() const;

			VkSemaphore& get();
			VkSemaphore const& get() const ;

			VkSemaphore& operator*() { return get(); }
			VkSemaphore const& operator*() const { return get(); }

		private:
			Semaphore(VkSemaphore semaphore);

		private:
			VkSemaphore _semaphore;
	};
}
