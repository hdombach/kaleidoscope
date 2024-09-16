#pragma once

#include <functional>

namespace util {
	template<typename From>
		class filter_iterator {
		public:
			using Predicate = std::function<bool(const typename From::value_type&)>;
			using value_type = typename From::value_type;
			using reference = typename From::reference;

			explicit filter_iterator(From begin, From end, Predicate pred):
				_begin(begin),
				_end(end),
				_pred(pred) {
					while (_begin != _end && !_pred(*_begin)) {
						_begin++;
					}
				}

		filter_iterator& operator++() {
			do {
				_begin++;
			} while (_begin != _end && !_pred(*_begin));
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
			Predicate _pred;
	};
}
