#include <string>

#include "KError.hpp"
#include "util/format.hpp"
#include "BaseError.hpp"

KError::KError():
	_type(Type::UNKNOWN)
{ }

KError::KError(Type type, std::string what, FLoc loc):
	_type(type),
	_what(what),
	_vk_error(VK_ERROR_UNKNOWN),
	_loc(loc)
{
	//_what = util::f("[KERROR ", loc.file_name(), "(", loc.line(), ":", loc.column(), ")]: ", what);
}

KError::KError(VkResult result, FLoc loc):
	_vk_error(result), _what(std::string("VkResult: ") + std::to_string(result)), _loc(loc)
{
	_what = util::f(
		"[", loc, "]: ",
		"VKResult(", result, ")"
	);
}

KError::KError(BaseError const &err, FLoc loc):
	_type(BASE_ERROR),
	_what(err.str()),
	_vk_error(VK_ERROR_UNKNOWN),
	_loc(loc)
{ }

KError KError::texture_exists(std::string texture_name, FLoc loc) {
	return KError(
		TEXTURE_EXISTS,
		util::f("Texture ", texture_name, " already exists"),
		loc
	);
}

KError KError::file_doesnt_exist(std::string file_name, FLoc loc) {
	return KError(
		FILE_DOESNT_EXIST,
		util::f("File ", file_name, " doesn't exist"),
		loc
	);
}

KError KError::invalid_image_file(std::string filename, FLoc loc) {
	return KError(
		INVALID_IMAGE_FILE,
		util::f("File ", filename, " is not a valid image"),
		loc
	);
}

KError KError::invalid_mesh_file(std::string filename, FLoc loc) {
	return KError(
		INVALID_MESH_FILE,
		util::f("File ", filename, " is not a valid mesh"),
		loc
	);
}

KError KError::invalid_node(uint32_t id, FLoc loc) {
	return KError(
		INVALID_NODE,
		util::f("Invalid node: ", id),
		loc
	);
}

KError KError::mesh_already_exists(std::string mesh_name, FLoc loc) {
	return KError(
		MESH_ALREADY_EXISTS,
		util::f("Mesh ", mesh_name, " already exists"),
		loc
	);
}

KError KError::material_already_exists(std::string material_name, FLoc loc) {
	return KError(
		MATERIAL_ALREADY_EXISTS,
		util::f("Material ", material_name, " already exists"),
		loc
	);
}

KError KError::texture_doesnt_exist(uint32_t id, FLoc loc) {
	return KError(
		TEXTURE_DOESNT_EXIST,
		util::f("Texture ", id, " doesn't exist"),
		loc
	);
}

KError KError::name_already_exists(std::string name, FLoc loc) {
	return KError(
		NAME_ALREADY_EXISTS,
		util::f("Identifier ", name, " already exists"),
		loc
	);
}

KError KError::invalid_mem_property(FLoc loc) {
	return KError(
		INVALID_MEM_PROPERTY,
		util::f("Invalid memory property"),
		loc
	);
}

KError KError::shader_compile_error(std::string msg, FLoc loc) {
	return KError(
		SHADER_COMPILER_ERROR,
		util::f("Shader compile error: ", msg),
		loc
	);
}

KError KError::empty_buffer(FLoc loc) {
	return KError(
		EMPTY_BUFFER,
		"Empty buffer",
		loc
	);
}

KError KError::invalid_arg(std::string msg, FLoc loc) {
	return KError(
		INVALID_ARG,
		util::f("Internal invalid argument: ", msg),
		loc
	);
}

KError KError::internal(std::string msg, FLoc loc) {
	return KError(
		INTERNAL,
		util::f("Internal error: ", msg),
		loc
	);
}

KError KError::codegen(std::string msg, FLoc loc) {
	return KError(
		CODEGEN,
		util::f("Codegen error: ", msg),
		loc
	);
}
