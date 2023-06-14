#pragma once
#include <vulkan/vulkan.h>
#include "vulkan/vulkan_core.h"


namespace vulkan {
	/*
	 * A texture interface
	 */
	class Texture {
		public:
			virtual ~Texture() = default;
			virtual VkDescriptorSet getDescriptorSet() const = 0;
	};
}
