#pragma once

#include "util/Util.hpp"
#include <map>
namespace util {
	template<typename From, typename P = util::has_value>
		class filter_iterator {
			public:
				using Pred = P;
			public:
				using value_type = typename From::value_type;
				using reference = typename From::reference;

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
