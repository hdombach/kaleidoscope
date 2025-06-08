#pragma once

#include <cmath>
#include <ostream>
#include <vector>
#include <iomanip>

#include "RectSize.hpp"

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
	inline std::ostream &operator<<(std::ostream &os, ptable const &t) { return t.print(os); }

	RectSize str_rect(std::string const &str);

	template<typename T>
		class plist {
			public:
				plist(std::vector<T> const &list): _l(list) {}

				std::ostream &print(std::ostream &os) const {
					os << "[";
					for (uint32_t i = 0; i < _l.size(); i++) {
						if (i) os << ", ";
						os << _l[i];
					}
					os << "]";
					return os;
				}
			private:
				std::vector<T> const &_l;
		};
	template<typename T>
		inline std::ostream &operator<<(std::ostream &os, plist<T> const &l) { return l.print(os); }

	/**
	 * @brief Prints a vector as a numberd list
	 */
	template<typename T>
		class plist_enumerated {
			public:
				plist_enumerated(std::vector<T> const &list): _l(list) {};

				std::ostream &print(std::ostream &os) const {
					uint32_t length = std::ceil(std::log10(_l.size()));
					for (int i = 0; i < _l.size(); i++) {
						os << std::setfill(' ') << std::setw(length) << i << ") ";
						os << _l[i] << std::endl;
					}
				}

			private:
				std::vector<T> const &_l;
		};

}
