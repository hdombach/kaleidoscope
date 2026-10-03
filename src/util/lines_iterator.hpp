#pragma once

#include <string_view>
#include <string>

namespace util {
	class lines_iterator {
		public:
			using value_type = std::string_view;
			using difference_type = std::ptrdiff_t;

			lines_iterator() = default;

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
					_next_newline();
				}
				return *this;
			}

			lines_iterator operator++(int) {
				auto r = *this;
				++(*this);
				return r;
			}

			value_type operator[](uint32_t i) {
				auto r = *this;
				while (i > 0) {
					r++;
					i--;
				}
				return *r;
			};

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
			void _next_newline() {
				while (*_end != '\0') {
					_end++;
					if (*_end == '\n') return;
				}
			}
	};

	static_assert(std::input_or_output_iterator<lines_iterator>);
	static_assert(std::sentinel_for<lines_iterator, lines_iterator>);

	inline auto get_lines(std::string const &str) {
		return std::ranges::subrange{lines_iterator::begin(str), lines_iterator::end(str)};
	}
}
