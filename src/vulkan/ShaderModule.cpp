#include "ShaderModule.h"
#include "Device.h"
#include "Error.h"
#include "file.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>

namespace vulkan {
	ShaderModule::ShaderModule(const char *filename, SharedDevice device): device_(device) {
		auto code = util::readEnvFile(filename);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

		auto result = vkCreateShaderModule(device->raw(), &createInfo, nullptr, &shaderModule_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	ShaderModule::ShaderModule(VkShaderModule shaderModule, SharedDevice device): shaderModule_(shaderModule), device_(device) {}

	ShaderModule::ShaderModule(ShaderModule&& other) {
		shaderModule_ = other.shaderModule_;
		device_ = other.device_;
		other.shaderModule_ = nullptr;
	}

	ShaderModule &ShaderModule::operator=(ShaderModule && other) {
		shaderModule_ = other.shaderModule_;
		device_ = other.device_;
		other.shaderModule_ = nullptr;

		return *this;
	}

	ShaderModule::~ShaderModule() {
		if (shaderModule_ != nullptr) {
			vkDestroyShaderModule(device_->raw(), shaderModule_, nullptr);
		}
	}

	VkShaderModule & ShaderModule::operator*() {
		return shaderModule_;
	}

	VkShaderModule & ShaderModule::raw() {
		return shaderModule_;
	}
}
