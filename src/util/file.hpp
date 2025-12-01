#pragma once

#include <filesystem>
#include <string>

#include "result.hpp"
#include "KError.hpp"
#include "BaseError.hpp"

namespace util {
	static const char * ENV_PATH = "KALEIDOSCOPE_PATH";

	util::Result<std::string, BaseError> env_file_path(std::string resource_name);

	std::string readEnvFile(std::string filename);

	std::string readFile(std::filesystem::path path);
	std::string readFile(std::ifstream &file);
}
