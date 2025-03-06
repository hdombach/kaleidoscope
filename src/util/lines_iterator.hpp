#pragma once

#include "util/IterAdapter.hpp"
#include <string_view>
#include <string>

namespace util {
	class lines_iterator {
		public:
			using value_type = std::string_view;

			static lines_iterator begin(std::string const &str) {
				lines_iterator r;
				r._begin = r._end = str.begin().base();
				r._next_newline();
				return r;
			}

			static lines_iterator end(std::string const &str) {
				lines_iterator r;
				r._begin = r._end = str.end().base();
				return r;
			}

			lines_iterator& operator++() {
				_begin = _end;
				if (*_begin != '\0') {
					_begin++;
					_end++;
					_next_newline();
				}
				return *this;
			}

			lines_iterator operator++(int) {
				auto r = *this;
				(*this)++;
				return r;
			}

			bool operator==(lines_iterator const &other) const {
				return _begin == other._begin;
			}
			bool operator!=(lines_iterator const &other) const {
				return _begin != other._begin;
			}

			value_type operator*() const { return std::string_view(_begin, _end); }


		private:
			const char *_begin;
			const char *_end;

		private:
			lines_iterator() = default;

			void _next_newline() {
				while (*_end != '\0') {
					_end++;
					if (*_end == '\n') return;
				}
			}
	};

	inline auto get_lines(std::string const &str) {
		return Adapt(lines_iterator::begin(str), lines_iterator::end(str));
	}
}
