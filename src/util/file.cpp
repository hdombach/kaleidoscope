#include "file.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sstream>

namespace util {

	std::string readFile(std::ifstream &file) {
		std::string result;

		file.seekg(0, std::ios::end);
		result.reserve(file.tellg());
		file.seekg(0, std::ios::beg);

		result.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

		return result;
	}

	std::string readEnvFile(std::string filename) {
		const char * envPathValue = std::getenv(ENV_PATH);
		if (envPathValue == nullptr) {
			envPathValue = "~";
		}
		std::stringstream envPathIterator;
		envPathIterator << envPathValue;
		std::string path;
		while(std::getline(envPathIterator, path, ':')) {
			std::string full_path = path + "/" + filename;
			std::ifstream file(full_path);

			if (!file.is_open()) {
				continue;
			}

			auto result = readFile(file);
			file.close();
			return result;
		}
		throw std::runtime_error("Could not find file " + path);
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
