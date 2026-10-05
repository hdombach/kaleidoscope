#pragma once

#include <ranges>
#include <vector>
#include "log.hpp"
#include "serial/Object.hpp"
#include "util/Util.hpp"
#include "util/get_iterator.hpp"

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

	template<typename GetId, typename Element>
	concept IdTrait = requires(Element const &el) {
		{ GetId()(el) } -> std::convertible_to<uint32_t>;
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
	template<std::default_initializable E, IdTrait<E> I = id_trait>
	class UIDList {
		public:
			/**
			 * @brief Is the specified element being used at the moment.
			 *   - true: In use
			 *   - false: Free to use or reserved at id 0
			 */
			struct Pred {
				UIDList<E, I> const *_list = nullptr;

				Pred() = default;
				Pred(UIDList<E, I> const *list): _list(list) {}

				bool operator()(E const &el) const {
					auto id = &el - _list->_elements.data();
					if (id == 0) return false;
					for (auto &e : _list->_empty) {
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
			using View = std::ranges::filter_view<std::ranges::ref_view<Container>, Pred>;
			using ConstView = std::ranges::filter_view<std::ranges::ref_view<const Container>, Pred>;
			using iterator = util::get_iterator<View>;
			using const_iterator = util::get_iterator<ConstView>;

		public:
			UIDList() = default;

			iterator begin() {
				return _view.begin();
			}
			iterator end() {
				return _view.end();
			}

			const_iterator begin() const {
				return _cview.begin();
			}
			const_iterator end() const {
				return _cview.end();
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

				return Pred(this)(_elements[id]);
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
					_recreate_view();
					return true;
				}

				// Check whether it is a duplicate
				if (contains(id)) {
					_recreate_view();
					return false;
				}

				_elements[id] = element;

				// Remove item from empty
				for (int i = 0; i < _empty.size(); i++) {
					if (_empty[i] == id) {
						_empty.erase(_empty.begin() + i);
					} else if (_empty[i] > id) {
						break;
					}
				}

				_recreate_view();
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
					_recreate_view();
					return true;
				}

				// Check whether it is a duplicate
				if (contains(id)) {
					_recreate_view();
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

				_recreate_view();
				return true;
			}

			/**
			 * @brief
			 *
			 * Returns true on success
			 */
			bool remove(uint32_t id) {
				if (!contains(id)) {
					_recreate_view();
					return false;
				}

				_elements[id] = Element();
				for (auto b = _empty.begin(); b < _empty.end(); b++) {
					if (*b > id) {
						_empty.insert(b, id);
						_recreate_view();
						return true;
					}
				}
				_empty.push_back(id);
				_recreate_view();
				return true;

			}

			Element &get(uint32_t id) { return _elements[id]; }
			Element const &get(uint32_t id) const { return _elements[id]; }

			Element &operator[](uint32_t id) { return get(id); }
			Element const &operator[](uint32_t id) const { return get(id); }

			size_t size() const { return _elements.size(); }

			void clear() {
				_elements.clear();
				_empty.clear();
				_recreate_view();
			}

		private:
			// Container of elements. Element 0 is reserved for an invalid element.
			Container _elements;
			/**
			 * @brief Sorted list of unused element indexes/ids
			 */
			std::vector<uint32_t> _empty;

			/**
			 * Filter view into the elements
			 */
			View _view = _elements | std::views::filter(Pred(this));
			mutable ConstView _cview = std::as_const(_elements) | std::views::filter(Pred(this));

			void _recreate_view() {
				_view = _elements | std::views::filter(Pred(this));
				_cview = std::as_const(_elements) | std::views::filter(Pred(this));
			}
	};
}
