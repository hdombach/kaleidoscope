#pragma once

#include <vulkan/vulkan_core.h>

#include "../util/result.hpp"

namespace vulkan {
	class Fence {
		public:
			static util::Result<Fence, VkResult> create();
			static util::Result<Fence, VkResult> create(VkDevice device);

			Fence();

			Fence(const Fence& other) = delete;
			Fence(Fence &&other);
			Fence& operator=(const Fence& other) = delete;
			Fence& operator=(Fence&& other);

			void destroy();
			~Fence() { destroy(); }

			operator bool() const;

			VkFence& get();
			VkFence const& get() const;

			VkFence& operator*() { return get(); }
			VkFence const& operator*() const { return get(); }

			VkResult wait();
			VkResult reset();

		private:
			Fence(VkFence fence);

		private:
			VkFence _fence;
	};
}
