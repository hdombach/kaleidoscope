#pragma once

#include <vulkan/vulkan.h>

namespace vulkan {
	class DescriptorPool {
		public:
			DescriptorPool();
			~DescriptorPool();

			VkDescriptorPool const &descriptorPool() const;
		private:
			VkDescriptorPool descriptorPool_;
	};
}
