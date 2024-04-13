#pragma once

#include <vulkan/vulkan.h>

namespace vulkan {
	class DescriptorPool {
		public:
			DescriptorPool();
			~DescriptorPool();

			VkDescriptorPool const &descriptor_pool() const;
		private:
			VkDescriptorPool _descriptor_pool;
	};
}
