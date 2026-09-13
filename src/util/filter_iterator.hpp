#pragma once

#include "util/Util.hpp"
#include <map>
#include <iterator>

namespace util {
	template<typename Pred, typename From>
	concept Predecessor = requires(From const &from, Pred const &pred) {
		{pred(*from)} -> std::convertible_to<bool>;
	};

	template<std::incrementable From, Predecessor<From> P = util::has_value>
		class filter_iterator {
			public:
				using Pred = P;
				using value_type = typename From::value_type;
				using reference = typename From::reference;
				using difference_type = std::ptrdiff_t;
			public:

				filter_iterator() = default;
				filter_iterator(filter_iterator const &other):
					_begin(other._begin),
					_end(other._end),
					_pred(other._pred)
				{}
				explicit filter_iterator(From begin, From end):
					filter_iterator(begin, end, Pred())
				{ }

				explicit filter_iterator(From begin, From end, Pred pred):
					_begin(begin),
					_end(end),
					_pred(pred)
			{
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

				filter_iterator operator++(int) {
					auto ret = *this;
					++(*this);
					return ret;
				}

				bool operator==(filter_iterator const &other) const {
					return _begin == other._begin;
				}
				bool operator!=(filter_iterator const &other) const {
					return !(*this == other);
				}

				reference operator*() const { return *_begin; }

			private:
				From _begin;
				From _end;
				Pred _pred;
		};
}
