#pragma once

#include "vulkan/vulkan_core.h"
#include <string>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Shader {
		public:
			Shader(const std::string& code);
			static Shader fromEnvFile(std::string const &file);
			~Shader();

			VkShaderModule shaderModule() const;
		private:
			VkShaderModule shaderModule_;
	};
}
