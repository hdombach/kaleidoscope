#pragma once

#include <filesystem>

#include "result.hpp"
#include "errors.hpp"

namespace util {
	static const char * ENV_PATH = "KALEIDOSCOPE_PATH";

	util::Result<std::string, KError> env_file_path(std::string resource_name);

	std::string readEnvFile(std::string filename);

	std::string readFile(std::filesystem::path path);
	std::string readFile(std::ifstream &file);

}
