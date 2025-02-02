#pragma once

#include <exception>
#include <ostream>
#include <string>
#include <source_location>

#include <vulkan/vulkan_core.h>

#include "util/log.hpp"
#include "util/FileLocation.hpp"


class KError: public std::exception {
	private:
		using SLoc = std::source_location;
		using FLoc = util::FileLocation;
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

		KError();
		KError(Type type, std::string what, FLoc=SLoc::current());
		KError(VkResult result, FLoc=SLoc::current());

		const char* what() const noexcept override { return _what.c_str(); }

		Type type() const { return _type; }
		util::FileLocation loc() const { return _loc; }

		static KError texture_exists(std::string texture_name, FLoc=SLoc::current());
		static KError file_doesnt_exist(std::string file_name, FLoc=SLoc::current());
		static KError invalid_image_file(std::string filename, FLoc=SLoc::current());
		static KError invalid_mesh_file(std::string filename, FLoc=SLoc::current());
		static KError invalid_node(uint32_t id, FLoc=SLoc::current());
		static KError mesh_already_exists(std::string mesh_name, FLoc=SLoc::current());
		static KError material_already_exists(std::string material_name, FLoc=SLoc::current());
		static KError texture_doesnt_exist(uint32_t id, FLoc=SLoc::current());
		static KError name_already_exists(std::string name, FLoc=SLoc::current());
		static KError invalid_mem_property(FLoc=SLoc::current());
		static KError shader_compile_error(std::string msg, FLoc=SLoc::current());
		static KError empty_buffer(FLoc=SLoc::current());
		static KError invalid_arg(std::string msg, FLoc=SLoc::current());
		static KError internal(std::string msg, FLoc=SLoc::current());
		static KError codegen(std::string msg, FLoc=SLoc::current());

	private:
		Type _type;

		util::FileLocation _loc;
		std::string _what;
		VkResult _vk_error;
};

inline std::ostream &operator <<(std::ostream &os, KError const &error) {
	os << error.what();
	return os;
}

#define catch_kerror catch (KError const &err) { return {err}; }

inline std::ostream& log_fatal_error(KError const &err) {
	return log_fatal_error(err.loc()) << err;
}

inline std::ostream& log_error(KError const &err) {
	return log_error(err.loc()) << err;
}

