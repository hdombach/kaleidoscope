#include "shader.hpp"
#include "error.hpp"
#include "file.hpp"
#include "graphics.hpp"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	Shader::Shader(const std::string& code) {
		auto createInfo = VkShaderModuleCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		require(vkCreateShaderModule(
					Graphics::DEFAULT->device(),
					&createInfo,
					nullptr,
					&shaderModule_));
	}

	Shader Shader::fromEnvFile(std::string const &file) {
		return Shader(util::readEnvFile(file));
	}

	Shader::~Shader() {
		vkDestroyShaderModule(Graphics::DEFAULT->device(), shaderModule_, nullptr);
	}

	VkShaderModule Shader::shaderModule() const {
		return shaderModule_;
	}
}
