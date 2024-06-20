#pragma once

#include <string>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"

namespace vulkan {
	class Shader {
		public:
			Shader(const std::string& code);
			Shader(const std::vector<uint32_t> &code);
			static Shader from_env_file(std::string const &file);
			static util::Result<Shader, KError> from_source_code(std::string const &code);

			Shader(const Shader& other) = delete;
			Shader(Shader &&other);
			Shader& operator=(const Shader& other) = delete;
			Shader& operator=(Shader&& other);

			void destroy();
			~Shader();

			VkShaderModule shader_module() const;
		private:
			Shader(const uint32_t *code, size_t code_s);

			VkShaderModule _shader_module;
	};
}
