#pragma once

#include <vector>
#include "log.hpp"
#include "util/filter_iterator.hpp"
#include "util/Util.hpp"

namespace util {
	/**
	 * @brief Struct for retrieving id
	 */
	struct id_trait {
		template<typename T>
		uint32_t operator()(T const &t) {
			return t.id();
		}
	};

	struct id_deref_trait {
		template<typename T>
		uint32_t operator()(T const &t) {
			return t->id();
		}
	};

	/**
	 * @brief A list of unique id's
	 * The caller is in charge of setting the unique id's however
	 * this class does provide helper functions
	 * The first element (id == 0) will always be empty
	 *
	 * The element class needs to either contain an id function or impliment trait
	 * to retrieve id
	 */
	template<typename E, typename I = id_trait>
	class UIDList {
		private:
			/**
			 * @brief Is the specified element being used at the moment.
			 *   - true: In use
			 *   - false: Free to use or reserved at id 0
			 */
			struct Pred {
				UIDList<E, I> const &_list;

				bool operator()(E const &el) {
					auto id = &el - _list._elements.data();
					if (id == 0) return false;
					for (auto &e : _list._empty) {
						if (e == id) {
							return false;
						} else if (e > id) {
							return true;
						}
					}
					return true;
				}
			};

		public:
			using Element = E;
			using IdTrait = I;
			using Container = std::vector<Element>;
			using iterator = util::filter_iterator<typename Container::iterator, Pred>;
			using const_iterator = util::filter_iterator<typename Container::const_iterator, Pred>;

		public:

			iterator begin() {
				return iterator(_elements.begin(), _elements.end(), Pred(*this));
			}
			iterator end() {
				return iterator(_elements.end(), _elements.end(), Pred(*this));
			}

			const_iterator begin() const {
				return const_iterator(_elements.begin(), _elements.end(), Pred(*this));
			}
			const_iterator end() const {
				return const_iterator(_elements.end(), _elements.end(), Pred(*this));
			}

			Container &raw() { return _elements; }
			Container const &raw() const { return _elements; }

			/**
			 * @brief Finds an id that is not used
			 */
			uint32_t get_id() const {
				if (_empty.empty()) {
					return std::max(_elements.size(), static_cast<size_t>(1));
				} else {
					return _empty.back();
				}
			}

			bool contains(uint32_t id) const {
				if (id >= _elements.size() || id == 0) {
					return false;
				}

				return Pred(*this)(_elements[id]);
			}

			/**
			 * @brief Inserts element
			 *
			 * @returns false if duplicate
			 */
			bool insert(Element const &element) {
				uint32_t id = IdTrait()(element);
				return insert(element, id);
			}

			/**
			 * @brief Inserts element
			 *
			 * @returns false if duplicate
			 */
			bool insert(Element &&element) {
				uint32_t id = IdTrait()(element);
				return insert(std::move(element), id);
			}

			/**
			 * @brief Inserts element
			 *
			 * @returns false if duplicate
			 */
			bool insert(Element const &element, uint32_t id) {
				//Add needed empty.
				while (id + 1 > _elements.size()) {
					if (!_elements.empty()) {
						// Ignore first element since it is reserved
						_empty.push_back(_elements.size());
					}
					_elements.push_back(Element());
				}

				// Fast track the process if get_id is used.
				if (!_empty.empty() && _empty.back() == id) {
					_elements[_empty.back()] = element;
					_empty.pop_back();
					return true;
				}

				// Check whether it is a duplicate
				if (contains(id)) {
					return false;
				}

				_elements[id] = element;

				// Remove item from element
				for (int i = 0; i < _empty.size(); i++) {
					if (_empty[i] == id) {
						_empty.erase(_empty.begin() + i);
					} else if (_empty[i] > id) {
						break;
					}
				}

				return true;
			}

			/**
			 * @brief Inserts element
			 *
			 * @returns false if duplicate
			 */
			bool insert(Element &&element, uint32_t id) {
				//Add needed empty.
				while (id + 1 > _elements.size()) {
					if (!_elements.empty()) {
						// Ignore first element since it is reserved
						_empty.push_back(_elements.size());
					}
					_elements.push_back(Element());
				}

				// Fast track the process if get_id is used.
				if (!_empty.empty() && _empty.back() == id) {
					_elements[_empty.back()] = std::move(element);
					_empty.pop_back();
					return true;
				}

				// Check whether it is a duplicate
				if (contains(id)) {
					return false;
				}

				_elements[id] = std::move(element);

				// Remove item from element
				for (int i = 0; i < _empty.size(); i++) {
					if (_empty[i] == id) {
						_empty.erase(_empty.begin() + i);
					} else if (_empty[i] > id) {
						break;
					}
				}

				return true;
			}

			/**
			 * @brief
			 *
			 * Returns true on success
			 */
			bool remove(uint32_t id) {
				if (!contains(id)) {
					return false;
				}

				_elements[id] = Element();
				for (auto b = _empty.begin(); b < _empty.end(); b++) {
					if (*b > id) {
						_empty.insert(b, id);
						return true;
					}
				}
				_empty.push_back(id);
				return true;

			}

			Element &get(uint32_t id) { return _elements[id]; }
			Element const &get(uint32_t id) const { return _elements[id]; }

			Element &operator[](uint32_t id) { return get(id); }
			Element const &operator[](uint32_t id) const { return get(id); }

			size_t size() const { return _elements.size(); }

			void clear() { _elements.clear(); }

		private:
			// Container of elements. Element 0 is reserved for an invalid element.
			Container _elements;
			/**
			 * @brief Sorted list of unused element indexes/ids
			 */
			std::vector<uint32_t> _empty;
	};
}
