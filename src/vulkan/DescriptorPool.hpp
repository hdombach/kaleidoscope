#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace vulkan {
	class DescriptorPool {
		public:
			static DescriptorPool create(std::string const &debug_name);

			DescriptorPool();

			DescriptorPool(const DescriptorPool& other) = delete;
			DescriptorPool(DescriptorPool &&other);
			DescriptorPool& operator=(const DescriptorPool& other) = delete;
			DescriptorPool& operator=(DescriptorPool&& other);

			void destroy();

			~DescriptorPool();

			VkDescriptorPool const &descriptor_pool() const;
		private:
			VkDescriptorPool _descriptor_pool;
	};
}
