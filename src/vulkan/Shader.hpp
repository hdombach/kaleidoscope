#pragma once

#include "vulkan/vulkan_core.h"
#include <string>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Shader {
		public:
			Shader(const std::string& code);
			static Shader from_env_file(std::string const &file);
			~Shader();

			VkShaderModule shader_module() const;
		private:
			VkShaderModule _shader_module;
	};
}
