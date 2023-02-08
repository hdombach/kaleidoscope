#include "ShaderModule.h"
#include "../util.h"
#include "Device.h"
#include "Error.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>

namespace vulkan {
	ShaderModule::ShaderModule(const char *filename, SharedDevice device): device_(device) {
		auto code = util::readFile(filename);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

		auto result = vkCreateShaderModule(**device, &createInfo, nullptr, &shaderModule_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	ShaderModule::ShaderModule(VkShaderModule shaderModule, SharedDevice device): shaderModule_(shaderModule), device_(device) {}

	ShaderModule::~ShaderModule() {
		vkDestroyShaderModule(**device_, shaderModule_, nullptr);
	}

	VkShaderModule & ShaderModule::operator*() {
		return shaderModule_;
	}

	VkShaderModule & ShaderModule::raw() {
		return shaderModule_;
	}
}
