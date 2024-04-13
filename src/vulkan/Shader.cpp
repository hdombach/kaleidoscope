#include "Shader.hpp"
#include "error.hpp"
#include "../util/file.hpp"
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
					&_shader_module));
	}

	Shader Shader::from_env_file(std::string const &file) {
		return Shader(util::readEnvFile(file));
	}

	Shader::~Shader() {
		vkDestroyShaderModule(Graphics::DEFAULT->device(), _shader_module, nullptr);
	}

	VkShaderModule Shader::shader_module() const {
		return _shader_module;
	}
}
