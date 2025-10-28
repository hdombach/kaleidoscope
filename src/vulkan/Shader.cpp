#include <string>

#include <vulkan/vulkan_core.h>
#include <shaderc/shaderc.hpp>

#include "Shader.hpp"
#include "util/file.hpp"
#include "graphics.hpp"

namespace vulkan {
	Shader::Shader(const std::string &code):
		Shader(reinterpret_cast<const uint32_t*>(code.data()), code.size())
	{ }

	Shader::Shader(const std::vector<uint32_t> &code):
		Shader(code.data(), code.size() * sizeof(uint32_t))
	{}

	Shader Shader::from_env_file(std::string const &file_name) {
		return Shader(util::readEnvFile(file_name));
	}

	util::Result<Shader, KError> Shader::from_source_code(
			const std::string &code,
			Type type)
	{
		auto compiler = shaderc::Compiler();
		auto options = shaderc::CompileOptions();

		options.SetOptimizationLevel(shaderc_optimization_level_size);

		shaderc_shader_kind kind;
		if (type == Type::Vertex) {
			kind = shaderc_glsl_vertex_shader;
		} else if (type == Type::Fragment) {
			kind = shaderc_glsl_fragment_shader;
		} else if (type == Type::Compute) {
			kind = shaderc_glsl_compute_shader;
		} else {
			return KError::internal("Unknown shader type");
		}
		
		//TODO: not just fragment shader
		auto module = compiler.CompileGlslToSpv(code, kind, "codegen", options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
			return KError::shader_compile_error(module.GetErrorMessage());
		}

		auto srv_code = std::vector<uint32_t>{module.begin(), module.end()};
		return Shader(srv_code);
	}

	Shader::Shader(Shader &&other) {
		_shader_module = other._shader_module;
		other._shader_module = nullptr;
	}

	Shader& Shader::operator=(Shader&& other) {
		destroy();

		_shader_module = other._shader_module;
		other._shader_module = nullptr;

		return *this;
	}

	void Shader::destroy() {
		if (_shader_module) {
			vkDestroyShaderModule(Graphics::DEFAULT->device(), _shader_module, nullptr);
			_shader_module = nullptr;
		}
	}

	Shader::~Shader() {
		destroy();
	}

	VkShaderModule Shader::shader_module() const {
		return _shader_module;
	}

	Shader::Shader(const uint32_t *code, size_t code_s) {
		auto createInfo = VkShaderModuleCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code_s;
		createInfo.pCode = code;

		util::require(vkCreateShaderModule(
					Graphics::DEFAULT->device(),
					&createInfo,
					nullptr,
					&_shader_module));

	}
}
