#include <string>

#include "KError.hpp"
#include "util/format.hpp"

KError::KError(Type type, std::string what):
	_type(type), _what(what), _vk_error(VK_ERROR_UNKNOWN)
{}

KError::KError(VkResult result):
	_vk_error(result), _what(std::string("VkResult: ") + std::to_string(result))
{}

KError KError::texture_exists(std::string texture_name) {
	return KError(
		TEXTURE_EXISTS,
		util::f("Texture ", texture_name, " already exists")
	);
}

KError KError::file_doesnt_exist(std::string file_name) {
	return KError(
		FILE_DOESNT_EXIST,
		util::f("File ", file_name, " doesn't exist")
	);
}

KError KError::invalid_image_file(std::string filename) {
	return KError(
		INVALID_IMAGE_FILE,
		util::f("File ", filename, " is not a valid image")
	);
}

KError KError::invalid_mesh_file(std::string filename) {
	return KError(
		INVALID_MESH_FILE,
		util::f("File ", filename, " is not a valid mesh")
	);
}

KError KError::invalid_node(uint32_t id) {
	return KError(
		INVALID_NODE,
		util::f("Invalid node: ", id)
	);
}

KError KError::mesh_already_exists(std::string mesh_name) {
	return KError(
		MESH_ALREADY_EXISTS,
		util::f("Mesh ", mesh_name, " already exists")
	);
}

KError KError::material_already_exists(std::string material_name) {
	return KError(
		MATERIAL_ALREADY_EXISTS,
		util::f("Material ", material_name, " already exists")
	);
}

KError KError::texture_doesnt_exist(uint32_t id) {
	return KError(
		TEXTURE_DOESNT_EXIST,
		util::f("Texture ", id, " doesn't exist")
	);
}

KError KError::name_already_exists(std::string name) {
	return KError(
		NAME_ALREADY_EXISTS,
		util::f("Identifier ", name, " already exists")
	);
}

KError KError::invalid_mem_property() {
	return KError(
		INVALID_MEM_PROPERTY,
		util::f("Invalid memory property")
	);
}

KError KError::shader_compile_error(std::string msg) {
	return KError(
		SHADER_COMPILER_ERROR,
		util::f("Shader compile error: ", msg)
	);
}

KError KError::empty_buffer() {
	return KError(
		EMPTY_BUFFER,
		"Empty buffer"
	);
}

KError KError::invalid_arg(std::string msg) {
	return KError(
		INVALID_ARG,
		util::f("Internal invalid argument: ", msg)
	);
}

KError KError::internal(std::string msg) {
	return KError(
		INTERNAL,
		util::f("Internal error: ", msg)
	);
}

KError KError::codegen(std::string msg) {
	return KError(
		CODEGEN,
		util::f("Codegen error: ", msg)
	);
}
