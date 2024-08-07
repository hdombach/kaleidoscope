#pragma once

#include "vulkan/vulkan_core.h"
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>

namespace vulkan {
	class VulkanError: public std::runtime_error {
		public:
			VulkanError(VkResult result): std::runtime_error(create_msg(result)) {}

		private:
			static std::string create_msg(VkResult result) {
				std::stringstream ss;
				ss << "Vulkan runtime error " << result << std::endl;
				return ss.str();
			}

			VkResult result;
	};
	inline void require(VkResult result) {
		if (result != VK_SUCCESS) {
			throw vulkan::VulkanError(result);
		}
	}
}
