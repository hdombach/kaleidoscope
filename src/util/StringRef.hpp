#pragma once

#include "util/FileLocation.hpp"
#include "util/log.hpp"
#include <string>

namespace util {
	/**
	 * @brief Contains a reference into string for generating FileLocations
	 */
	class StringRef {
		public:
			StringRef();

			StringRef(const char *str, const char *filename);

			StringRef(
				const char *str,
				uint32_t start=0,
				uint32_t end=0,
				const char *filename="UNNAMED"
			);

			bool empty() const;
			void inc(uint32_t count = 1);
			StringRef &concat(StringRef const &rhs);
			char get(uint32_t offset = 0) const;
			FileLocation location() const;
			StringRef dup(uint32_t offset) const;
			StringRef& set_size(uint32_t size);
			uint32_t size() const;
			StringRef& set_filename(const char *filename);
			std::string str() const;
			StringRef substr(uint32_t s, uint32_t e) const;
			StringRef operator+(uint32_t offset) const;
			StringRef &operator++();
			StringRef &operator+=(uint32_t count);
			StringRef operator+(StringRef const &rhs) const;
			StringRef &operator+=(StringRef const &rhs);
			char operator*() const;
			char operator[](uint32_t offset);

		private:
			uint32_t _start;
			uint32_t _end;
			uint32_t _line;
			uint32_t _column;
			const char *_str;
			const char *_filename;
	};
}
