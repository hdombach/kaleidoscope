#pragma once

#include <filesystem>
namespace util {
	static const char * ENV_PATH = "KALEIDOSCOPE_PATH";

	std::string env_file_path(std::string resource_name);

	std::string readEnvFile(std::string filename);

	std::string readFile(std::filesystem::path path);
	std::string readFile(std::ifstream &file);

}
