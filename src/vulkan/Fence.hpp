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

			~Fence();

			VkFence& value();
			VkFence const& value() const;

			VkFence& operator*() { return value(); }
			VkFence const& operator*() const { return value(); }

			VkResult wait();
			VkResult reset();

		private:
			Fence(VkFence fence);

		private:
			VkFence _fence;
	};
}
