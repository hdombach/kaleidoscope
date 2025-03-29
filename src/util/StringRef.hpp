#pragma once

#include "util/FileLocation.hpp"
#include <string>

namespace util {
	/**
	 * @brief Contains a reference into string for generating FileLocations
	 */
	class StringRef {
		public:
			StringRef():
				_str(nullptr),
				_filename(nullptr),
				_i(0),
				_line(1),
				_column(1)
			{}

			StringRef(const char *str, const char *filename):
				_str(str),
				_filename(filename),
				_i(0),
				_line(1),
				_column(1)
			{}

			void inc(uint32_t count = 1) {
				for (uint32_t i = 0; i < count; i++) {
					if (_str[_i] == '\0') {
						return;
					}
					_i++;
					_column++;
					if (_str[_i] == '\n') {
						_column = 0;
						_line++;
					}
				}
			}

			char get(uint32_t offset = 0) const {
				return _str[_i + offset];
			}
			FileLocation location() const {
				return FileLocation(_line, _column, _filename);
			}
			StringRef dup(uint32_t offset) const {
				StringRef result = *this;
				result += offset;
				return result;
			}
			const char *str() const {
				return _str + _i;
			}
			uint32_t cur_offset() const {
				return _i;
			}

			StringRef operator+(uint32_t offset) const {
				return dup(offset);
			}
			StringRef &operator++() {
				inc();
				return *this;
			}
			StringRef &operator+=(uint32_t count) {
				inc(count);
				return *this;
			}
			char operator*() const {
				return get();
			}
			char operator[](uint32_t offset) {
				return get(offset);
			}

		private:
			uint32_t _i;
			uint32_t _line;
			uint32_t _column;
			const char *_str;
			const char *_filename;
	};
}
