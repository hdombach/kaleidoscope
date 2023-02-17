#include "ShaderModule.h"
#include "Device.h"
#include "Error.h"
#include "file.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>

namespace vulkan {
	void ShaderModuleDeleter::operator()(ShaderModuleData *data) const {
		vkDestroyShaderModule(data->device_->raw(), data->shaderModule_, nullptr);
		delete data;
	}

	ShaderModule::ShaderModule(const char *filename, SharedDevice device):
		base_type(new ShaderModuleData{nullptr})
	{
		auto code = util::readEnvFile(filename);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

		{
			auto data = get();
			auto result = vkCreateShaderModule(device->raw(), &createInfo, nullptr, &data->shaderModule_);
			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}
			data->device_ = device;
		}
	}

	VkShaderModule & ShaderModule::raw() {
		return get()->shaderModule_;
	}
}
