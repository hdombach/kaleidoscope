#include "StringRef.hpp"
#include "util/log.hpp"

namespace util {
	StringRef::StringRef():
		_str(nullptr),
		_filename(nullptr),
		_start(0),
		_end(0),
		_line(1),
		_column(0)
	{}

	StringRef::StringRef(const char *str, const char *filename):
		_str(str),
		_filename(filename),
		_start(0),
		_end(0),
		_line(1),
		_column(1)
	{}

	StringRef::StringRef(
		const char *str,
		uint32_t start,
		uint32_t end,
		const char *filename
	):
		_str(str),
		_filename(filename),
		_start(0),
		_end(0),
		_line(1),
		_column(1)
	{
		inc(start);
		//TODO: validation
		_end = end;
	}

	bool StringRef::empty() const {
		return _str == nullptr || *_str == '\0';
	}

	void StringRef::inc(uint32_t count) {
		for (uint32_t i = 0; i < count; i++) {
			if (_str[_start] == '\0') {
				return;
			}
			_start++;
			_column++;
			if (_str[_start] == '\n') {
				_column = 0;
				_line++;
			}
		}
		if (_start > _end) _end = 0;
	}

	StringRef &StringRef::concat(StringRef const &rhs) {
		log_assert(_str == rhs._str, "Cannot concat strings that have seperate source strings.");
		log_assert(_end == rhs._start, "Cannot concat strings that are not adjacent.");
		_end = rhs._end;
		return *this;
	}

	char StringRef::get(uint32_t offset) const {
		return _str[_start + offset];
	}

	FileLocation StringRef::location() const {
		return FileLocation(_line, _column, _filename);
	}

	StringRef StringRef::dup(uint32_t offset) const {
		StringRef result = *this;
		result += offset;
		return result;
	}

	StringRef& StringRef::set_size(uint32_t size) {
		_end = _start + size;
		return *this;
	}

	uint32_t StringRef::size() const {
		if (_end) {
			return _end - _start;
		} else {
			return strlen(_str + _start);
		}
	}

	StringRef& StringRef::set_filename(const char *filename) {
		_filename = filename;
		return *this;
	}

	std::string_view StringRef::str() const {
		if (empty()) return std::string_view();
		try {
			if (_end) {
				return std::string_view(_str + _start, _str + _end);
			} else {
				return std::string_view(_str + _start);
			}
		} catch (...) {
			log_error() << "couldn't create string" << std::endl;
			return "error";
		}
	}

	StringRef StringRef::substr(uint32_t s, uint32_t e) const {
		return dup(s).set_size(e);
	}

	StringRef StringRef::operator+(uint32_t offset) const {
		return dup(offset);
	}

	StringRef &StringRef::operator++() {
		inc();
		return *this;
	}

	StringRef &StringRef::operator+=(uint32_t count) {
		inc(count);
		return *this;
	}

	StringRef StringRef::operator+(StringRef const &rhs) const {
		auto r = *this;
		r += rhs;
		return r;
	}

	StringRef &StringRef::operator+=(StringRef const &rhs) {
		concat(rhs);
		return *this;
	}

	char StringRef::operator*() const {
		return get();
	}

	char StringRef::operator[](uint32_t offset) {
		return get(offset);
	}
}
