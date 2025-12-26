#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sstream>

#include "file.hpp"
#include "result.hpp"
#include "util/Env.hpp"
#include "util/log.hpp"

namespace util {

	util::Result<std::string, BaseError> env_file_path(std::string filename) {
		if (std::filesystem::exists(filename)) {
			return filename;
		}
		const char * envPathValue = std::getenv(ENV_PATH);
		if (envPathValue == nullptr) {
			envPathValue = "~";
		}
		std::stringstream envPathIterator;
		envPathIterator << envPathValue;
		std::string path;
		while(std::getline(envPathIterator, path, ':')) {
			auto full_path = std::filesystem::path(path) / filename;

			if (std::filesystem::exists(full_path)) {
				return {full_path};
			}
		}

		{
			auto full_path = util::g_env.working_dir / filename;

			if (std::filesystem::exists(full_path)) {
				return {full_path};
			}
		}
		return BaseError(util::f("File ", filename, " doesn't exist"));
	}

	std::string readFile(std::ifstream &file) {
		std::string result;

		file.seekg(0, std::ios::end);
		result.reserve(file.tellg());
		file.seekg(0, std::ios::beg);

		result.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

		return result;
	}

	std::string readEnvFile(std::string filename) {
		//TODO: propagate error
		if (auto path = env_file_path(filename)) {
			return readFile(path.value());
		} else {
			log_error() << "Problem opening file " << filename << ":\n" << path.error();
			return "";
		}
	}

	std::string readFile(std::filesystem::path path) {
		std::ifstream file(path);

		if (!file.is_open()) {
			std::stringstream ss;
			ss << "failed to open file \"" << path.string() << "\"";
			throw std::runtime_error(ss.str());
		}

		auto result = readFile(file);

		file.close();

		return result;
	}
}
