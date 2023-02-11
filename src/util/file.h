#pragma once

#include <filesystem>
namespace util {
	static const char * ENV_PATH = "KALEIDOSCOPE_PATH";

	std::string readEnvFile(std::string filename);

	std::string readFile(std::filesystem::path path);
	std::string readFile(std::ifstream &file);
}
