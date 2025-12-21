#pragma once

#include <vector>
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
	template<typename Element, typename Pred = util::has_value, typename IdTrait = id_trait>
	class UIDList {
		public:
			using Container = std::vector<Element>;
			using iterator = util::filter_iterator<typename Container::iterator, Pred>;
			using const_iterator = util::filter_iterator<typename Container::const_iterator, Pred>;

			iterator begin() {
				return iterator(_elements.begin(), _elements.end());
			}
			iterator end() {
				return iterator(_elements.end(), _elements.end());
			}

			const_iterator begin() const {
				return const_iterator(_elements.begin(), _elements.end());
			}
			const_iterator end() const {
				return const_iterator(_elements.end(), _elements.end());
			}

			Container &raw() { return _elements; }
			Container const &raw() const { return _elements; }

			/**
			 * @brief Finds an id that is not used
			 */
			uint32_t get_id() const {
				uint32_t id;
				for (id = 1; id < _elements.size(); id++) {
					if (!Pred()(_elements[id])) {
						break;
					}
				}
				return id;
			}

			bool contains(uint32_t id) const {
				if (id >= _elements.size() || id == 0) {
					return false;
				}

				return Pred()(_elements[id]);
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
				if (contains(id)) {
					return false;
				}

				while (id + 1 > _elements.size()) {
					_elements.push_back(Element());
				}
				_elements[id] = element;

				return true;
			}

			/**
			 * @brief Inserts element
			 *
			 * @returns false if duplicate
			 */
			bool insert(Element &&element, uint32_t id) {
				if (contains(id)) {
					return false;
				}

				while (id + 1 > _elements.size()) {
					_elements.push_back(Element());
				}
				_elements[id] = std::move(element);

				return true;
			}

			Element &get(uint32_t id) { return _elements[id]; }
			Element const &get(uint32_t id) const { return _elements[id]; }

			Element &operator[](uint32_t id) { return get(id); }
			Element const &operator[](uint32_t id) const { return get(id); }

			size_t size() const { return _elements.size(); }

			void clear() { _elements.clear(); }

		private:
			Container _elements;
	};
}
