#pragma once

#include <cmath>
#include <ostream>
#include <vector>
#include <iomanip>

#include "RectSize.hpp"
#include "Util.hpp"

namespace util {
	/**
	 * @brief Object that prints table to ostream
	 */
	class ptable {
		public:
			ptable(std::vector<std::vector<std::string>> const &table): _table(table) {}

			std::ostream &print(std::ostream &os) const;
		private:
			std::vector<std::vector<std::string>> const &_table;
	};

	RectSize str_rect(std::string const &str);

	template<typename Container>
		class plist {
			public:
				plist(Container const &container): _c(container) {}

				std::ostream &print(std::ostream &os) const {
					os << "[";
					bool is_first = true;
					for (auto &element : _c) {
						if (is_first) {
							is_first = false;
						} else {
							os << ", ";
						}
						os << element;
					}
					os << "]";
					return os;
				}
			private:
				Container const &_c;
		};

	template<typename Container>
		class pllist {
			public:
				pllist(Container const &container): _c(container) {}

				std::ostream &print(std::ostream &os) const {
					os << "[";
					bool is_first = true;
					for (auto &element : _c) {
						if (is_first) {
							is_first = false;
						} else {
							os << ", ";
						}
						util::plist(element).print(os);
					}
					os << "]";
					return os;
				}
			private:
				Container const &_c;
		};


	/**
	 * @brief Prints a vector as a numberd list
	 */
	template<typename T>
		class plist_enumerated {
			public:
				plist_enumerated(std::vector<T> const &list, bool one_indexed = false):
					_l(list),
					_one_indexed(one_indexed)
				{};

				std::ostream &print(std::ostream &os) const {
					uint32_t length = std::ceil(std::log10(_l.size()));
					for (int i = 0; i < _l.size(); i++) {
						os << std::setfill(' ') << std::setw(length) << i + _one_indexed << ") ";
						os << util::trim(_l[i]) << std::endl;
					}
					return os;
				}

			private:
				std::vector<T> const &_l;
				bool _one_indexed;
		};

}

inline std::ostream &operator<<(std::ostream &os, util::ptable const &t) { return t.print(os); }

template<typename T>
	inline std::ostream &operator<<(std::ostream &os, util::plist<T> const &l) {
		return l.print(os);
	}

template<typename T>
inline std::ostream &operator<<(std::ostream &os, util::pllist<T> const &l) {
	return l.print(os);
}

template<typename T>
	inline std::ostream &operator<<(std::ostream &os, util::plist_enumerated<T> const &l) {
		return l.print(os);
	}

