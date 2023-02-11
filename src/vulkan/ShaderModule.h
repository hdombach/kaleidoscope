#pragma once

#include "Device.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	class ShaderModule {
		public:
			ShaderModule() = default;
			ShaderModule(const char *fileName, SharedDevice device);
			ShaderModule(VkShaderModule shaderModule, SharedDevice device);
			ShaderModule(const ShaderModule &) = delete;
			ShaderModule& operator=(const ShaderModule &) = delete;
			ShaderModule(ShaderModule&&);
			ShaderModule& operator=(ShaderModule &&);
			~ShaderModule();
			VkShaderModule & operator*();
			VkShaderModule & raw();

		private:
			VkShaderModule shaderModule_ = nullptr;

			SharedDevice device_;
	};
}
