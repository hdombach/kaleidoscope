#pragma once

#include <cstdint>
#include <source_location>
#include <string>
#include <filesystem>

#include "util/Env.hpp"

namespace util {
	/**
	 * @brief A location in either source file or code generated file
	 */
	struct FileLocation {
		std::uint32_t line=0;
		std::uint32_t column=0;
		std::string file_name;

		FileLocation() = default;

		FileLocation(
			std::uint32_t line,
			std::uint32_t column,
			std::string const &file_name
		): line(line), column(column), file_name(file_name) { }

		FileLocation(std::source_location const &loc):
			line(loc.line()), column(loc.column()), file_name(loc.file_name()) { }

		FileLocation& operator=(std::source_location const &loc) {
			line = loc.line();
			column = loc.column();
			file_name = loc.file_name();
			return *this;
		}

		void debug(std::ostream &os) const {
			os
				<< std::filesystem::path(file_name).filename().c_str()
				<< "(" << line << ":" << column << ")";
		}

		bool operator==(const FileLocation &other) const {
			return line == other.line && column == other.column && file_name == other.file_name;
		}
		bool operator!=(const FileLocation &other) const {
			return !(*this == other);
		}
	};
}

inline bool operator<(util::FileLocation const &lhs, util::FileLocation const &rhs) {
	if (lhs.line == rhs.line) {
		return lhs.column < rhs.column;
	} else {
		return lhs.line < rhs.line;
	}
}

inline bool operator>(util::FileLocation const &lhs, util::FileLocation const &rhs) {
	if (lhs.line == rhs.line) {
		return lhs.column > rhs.column;
	} else {
		return lhs.line > rhs.line;
	}
}

inline std::ostream &operator<<(std::ostream &os, util::FileLocation const &file) {
	file.debug(os);
	return os;
}
