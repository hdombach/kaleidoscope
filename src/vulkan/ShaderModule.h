#pragma once

#include "Device.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <memory>

namespace vulkan {
	
	struct ShaderModuleData {
		VkShaderModule shaderModule_;
		SharedDevice device_;
	};
	struct ShaderModuleDeleter {
		void operator()(ShaderModuleData *data) const;
	};

	class ShaderModule: std::unique_ptr<ShaderModuleData, ShaderModuleDeleter> {
		public:
			using base_type = std::unique_ptr<ShaderModuleData, ShaderModuleDeleter>;

			ShaderModule(const char *fileName, SharedDevice device);
			VkShaderModule & raw();
	};
}
