#pragma once

#include <exception>
#include <ostream>
#include <string>

#include <vulkan/vulkan_core.h>


class KError: public std::exception {
	public:
		enum Type {
			UNKNOWN,
			TEXTURE_EXISTS,
			FILE_DOESNT_EXIST,
			INVALID_IMAGE_FILE,
			INVALID_MESH_FILE,
			INVALID_NODE,
			MESH_ALREADY_EXISTS,
			MATERIAL_ALREADY_EXISTS,
			TEXTURE_DOESNT_EXIST,
			NAME_ALREADY_EXISTS,
			INVALID_MEM_PROPERTY,
			VK_ERROR,
			SHADER_COMPILER_ERROR,
			EMPTY_BUFFER,
			INVALID_ARG,
			INTERNAL,
			CODEGEN,
		};

		KError(Type type, std::string what);
		KError(VkResult result);

		const char* what() const noexcept override { return _what.c_str(); }

		Type type() const { return _type; }

		static KError texture_exists(std::string texture_name);
		static KError file_doesnt_exist(std::string file_name);
		static KError invalid_image_file(std::string filename);
		static KError invalid_mesh_file(std::string filename);
		static KError invalid_node(uint32_t id);
		static KError mesh_already_exists(std::string mesh_name);
		static KError material_already_exists(std::string material_name);
		static KError texture_doesnt_exist(uint32_t id);
		static KError name_already_exists(std::string name);
		static KError invalid_mem_property();
		static KError shader_compile_error(std::string msg);
		static KError empty_buffer();
		static KError invalid_arg(std::string msg);
		static KError internal(std::string msg);
		static KError codegen(std::string msg);

	private:
		Type _type;

		std::string _what;
		VkResult _vk_error;
};

inline std::ostream &operator <<(std::ostream &os, KError const &error) {
	os << error.what();
	return os;
}

#define catch_kerror catch (KError const &err) { return {err}; }

