#include <string>

#include "errors.hpp"

KError::KError(Type type, std::string content, std::string desc):
	_desc(desc), _content(content), _vk_error(VK_ERROR_UNKNOWN), _type(type)
{}

KError::KError(VkResult result):
	_vk_error(result)
{
	_desc = std::string("VkResult: ") + std::to_string(result);
}

std::string const &KError::desc() const {
	return _desc;
}

std::string const &KError::content() const {
	return _content;
}

KError KError::texture_exists(std::string texture_name) {
	return KError(
			TEXTURE_EXISTS,
			texture_name,
			"Texture " + texture_name + " already exists");
}

KError KError::file_doesnt_exist(std::string file_name) {
	return KError(
			FILE_DOESNT_EXIST,
			file_name,
			"File " + file_name + " doesn't exist");
}

KError KError::invalid_image_file(std::string filename) {
	return KError(
			INVALID_IMAGE_FILE,
			filename,
			"File " + filename + " is not a valid image");
}

KError KError::invalid_mesh_file(std::string filename) {
	return KError(
			INVALID_MESH_FILE,
			filename,
			"File " + filename + " is not a valid mesh");
}

KError KError::invalid_node(uint32_t id) {
	return KError(
			INVALID_NODE,
			"",
			"Invalid node: " + std::to_string(id));
}

KError KError::mesh_already_exists(std::string mesh_name) {
	return KError(
			MESH_ALREADY_EXISTS,
			mesh_name,
			"Mesh " + mesh_name + " already exists");
}

KError KError::material_already_exists(std::string material_name) {
	return KError(
			MATERIAL_ALREADY_EXISTS,
			material_name,
			"Material " + material_name + " already exists");
}

KError KError::texture_doesnt_exist(uint32_t id) {
	return KError(
			TEXTURE_DOESNT_EXIST,
			std::to_string(id),
			"Texture " + std::to_string(id) + " doesn't exist");
}

KError KError::name_already_exists(std::string name) {
	return KError(
			NAME_ALREADY_EXISTS,
			name,
			"Identifier " + name + " already exists");
}

KError KError::invalid_mem_property() {
	return KError(
			INVALID_MEM_PROPERTY,
			std::string(),
			"Invalid memory property");
}

KError KError::shader_compile_error(std::string msg) {
	return KError(
			SHADER_COMPILER_ERROR,
			msg,
			"Shader compile error");
}

KError KError::empty_buffer() {
	return KError(
			EMPTY_BUFFER,
			std::string(),
			"Empty buffer");
}

KError KError::invalid_arg(std::string msg) {
	return KError(
			INVALID_ARG,
			msg,
			"Internal invalid argument");
}

KError KError::internal(std::string msg) {
	return KError(
			INTERNAL,
			std::string(),
			msg);
}
