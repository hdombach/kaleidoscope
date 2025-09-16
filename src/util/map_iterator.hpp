#pragma once

#include <functional>

namespace util {
	template<typename FromIter, typename ToValue>
	class map_iterator {
		public:
			using Func = std::function<ToValue(FromIter)>;

			using value_type = ToValue;
			//TODO: will probably have to change in the future but temporarily
			//gets around the fact that you cannot get reference to a computed
			//value
			using reference = ToValue;

			explicit map_iterator(FromIter begin, Func func):
				_begin(begin),
				_func(func)
			{}

			map_iterator& operator++() {
				_begin++;
				return *this;
			}

			map_iterator operator++(int) {
				auto ret = *this;
				++(*this);
				return ret;
			}

			bool operator==(map_iterator const &other) const {
				return _begin == other._begin;
			}
			bool operator!=(map_iterator const &other) const {
				return !(*this == other);
			}

			ToValue operator*() const { return _func(_begin); }

		private:
			FromIter _begin;
			Func _func;
	};
}
