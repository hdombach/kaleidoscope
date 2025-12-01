#pragma once

#include <filesystem>
#include <string>

#include "result.hpp"
#include "KError.hpp"
#include "BaseError.hpp"

namespace util {
	static const char * ENV_PATH = "KALEIDOSCOPE_PATH";

	enum class FileErrorType {
		MISSING_FILE
	};

	using FileError = TypedError<FileErrorType>;

	util::Result<std::string, KError> env_file_path(std::string resource_name);

	std::string readEnvFile(std::string filename);

	std::string readFile(std::filesystem::path path);
	std::string readFile(std::ifstream &file);
}

template<>
const char *util::FileError::type_str(util::FileErrorType t);

std::ostream &operator<<(std::ostream &os, util::FileError err);
