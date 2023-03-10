#pragma once

#include "vulkan/vulkan_core.h"
#include <exception>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vulkan/vk_enum_string_helper.h>

namespace vulkan {
	class Error: public std::runtime_error {
		public:
			Error(VkResult result): std::runtime_error(create_msg(result)) {}

		private:
			static std::string create_msg(VkResult result) {
				std::stringstream ss;
				ss << "Vulkan runtime error " << string_VkResult(result) << std::endl;
				return ss.str();
			}

			VkResult result;
	};
}
