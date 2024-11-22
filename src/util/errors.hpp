#pragma once

#include <ostream>
#include <string>

#include <vulkan/vulkan_core.h>


class KError {
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
		};

		KError(Type type, std::string content, std::string desc);
		KError(VkResult result);

		/** @brief Long description that can be printed*/
		std::string const &desc_part() const;

		/** @brief Detail like texture name or filename */
		std::string const &content_part() const;

		std::string str() const { return desc_part() + ": " + content_part(); }

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

	private:
		Type _type;

		std::string _desc;
		std::string _content;
		VkResult _vk_error;
};

inline std::ostream &operator <<(std::ostream &os, KError const &error) {
	os << error.str();
	return os;
}
