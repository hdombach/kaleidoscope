#pragma once

#include <ostream>
#include <string>

#include <vulkan/vulkan_core.h>


class KError {
	public:
		enum Type {
			UNKNOWN,
			TEXTURE_EXISTS,
			INVALID_IMAGE_FILE,
			INVALID_MESH_FILE,
			MESH_ALREADY_EXISTS,
			MATERIAL_ALREADY_EXISTS,
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
		virtual std::string const &desc() const;

		/** @brief Detail like texture name or filename */
		virtual std::string const &content() const;

		Type type() const { return _type; }

		static KError texture_exists(std::string texture_name);
		static KError invalid_image_file(std::string filename);
		static KError invalid_mesh_file(std::string filename);
		static KError mesh_already_exists(std::string mesh_name);
		static KError material_already_exists(std::string material_name);
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
	os << error.desc() << ": " << error.content();
	return os;
}
