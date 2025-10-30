#pragma once

#include <string>

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "util/KError.hpp"

namespace vulkan {
	class Shader {
		public:
			enum class Type {
				Fragment,
				Vertex,
				Compute,
			};

			Shader() = default;
			Shader(const std::string& code);
			Shader(const std::vector<uint32_t> &code);
			static Shader from_env_file(std::string const &file_name);
			static util::Result<Shader, KError> from_source_code(
					std::string const &code,
					Type type);

			Shader(const Shader& other) = delete;
			Shader(Shader &&other);
			Shader& operator=(const Shader& other) = delete;
			Shader& operator=(Shader&& other);

			void destroy();
			~Shader();

			bool has_value() const;
			operator bool() const;

			VkShaderModule shader_module() const;
		private:
			Shader(const uint32_t *code, size_t code_s);

		private:
			VkShaderModule _shader_module = nullptr;
	};
}
