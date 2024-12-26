#pragma once

#include "util/Util.hpp"

namespace util {
	template<typename From, typename Pred = util::has_value<typename From::value_type>>
		class filter_iterator {
		public:
			using value_type = typename From::value_type;
			using reference = typename From::reference;

			explicit filter_iterator(From begin, From end):
				_begin(begin),
				_end(end)
			 {
					while (_begin != _end && !Pred()(*_begin)) {
						_begin++;
					}
				}

		filter_iterator& operator++() {
			do {
				_begin++;
			} while (_begin != _end && !Pred()(*_begin));
			return *this;
		}

		filter_iterator& operator++(int) {
			auto ret = *_begin;
			++(*this);
			return ret;
		}

		bool operator==(filter_iterator other) const {
			return _begin == other._begin;
		}
		bool operator!=(filter_iterator other) const {
			return !(*this == other);
		}

		reference operator*() const { return *_begin; }

		private:
			From _begin;
			From _end;
	};
}
