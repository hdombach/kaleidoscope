#pragma once

#include <concepts>
#include <vector>
#include "Object.hpp"
#include "Document.hpp"
#include "util/log.hpp"
#include "util/Util.hpp"
#include "util/UIDList.hpp"

namespace serial {
	template<typename T>
	class Vector: public Object {
		public:
			using Container = std::vector<T>;

			using reference = Container::reference;
			using const_reference = Container::const_reference;

			using iterator = Container::iterator;
			using const_iterator = Container::const_iterator;

			using size_type = Container::size_type;

			static const uint32_t TYPE_ID = 1;

			class TRemove: public Transaction {
				public:
					TRemove(size_type id): _id(id) {}

					static Ptr create(size_type id) {
						return new TRemove(id);
					}

					Ptr apply(Object &obj) {
						log_assert(obj.type_id() == TYPE_ID, util::f("Expecting object of type Vector but got ", obj.type_str()));
						Vector<T> &vobj = obj;

						vobj._ignore_start();
						auto element = std::move(vobj[_id]);
						vobj.remove(_id);
						vobj._ignore_end();

						return TInsert::create(_id, std::move(element));
					}

				private:
					size_type _id = 0;
			};

			class TInsert: public Transaction {
				public:
					TInsert(size_type idx, T &&value): _idx(idx), _value(std::move(value)) {}

					static Ptr create(size_type idx, T &&value) {
						return new TInsert(idx, std::move(value));
					}

					Ptr apply(Object &obj) {
						log_assert(obj.type_id() == TYPE_ID, util::f("Expecting object of type  Vector but got ", obj.type_str()));
						Vector<T> &vobj = obj;

						vobj._ignore_start();
						vobj.insert(_idx, std::move(_value));
						vobj._ignore_end();

						return TRemove::create(_idx);
					}

				private:
					size_type _idx;
					T _value;
			};

		public:
			Vector(std::vector<T> const &v): _v(v) {};
			Vector(std::vector<T> &&v): _v(v) {};

			uint32_t type_id() const { return TYPE_ID; }
			const char *type_str() const { return "Vector"; }

			reference operator[](size_type pos) { return _v[pos]; }
			const_reference operator[](size_type pos) const { return _v[pos]; }

			reference front() { return _v.front(); }
			const_reference front() const { return _v.front(); }

			reference back() { return _v.back(); }
			const_reference back() const { return _v.back(); }

			iterator begin() { return _v.begin(); }
			const_iterator begin() const { return _v.begin(); }

			iterator end() { return _v.end(); }
			const_iterator end() const { return _v.end(); }

			void push_back(T &&value) {
				insert(_v.size(), std::move(value));
			}

			void pop_back() {
				remove(_v.size() - 1);
			}

			void remove(size_t idx) {
				log_assert(idx >= 0 && idx < _v.size(), util::f("Remove idx ", idx, " is not within bounds"));

				_implicit_start();
				_add_transaction(TInsert::create(idx, std::move(_v[idx])));
				_v.erase(_v.begin() + idx);
				_implicit_end();
			}

			void insert(size_t idx, T &&value) {
				log_assert(idx >= 0 && idx <= _v.size(), util::f("Insert idx ", idx, " is not within the bounds"));

				_implicit_start();
				_add_transaction(TRemove::create(idx));
				_v.insert(_v.begin() + idx, std::move(value));
				_implicit_end();
			}

		private:
			Container _v;
	};

	template<std::derived_from<Object> T>
	class UIDList: public Object {
		public:
			static const uint32_t TYPE_ID = 2;

			class TRemove: public Transaction {
				public:
					TRemove(size_t id): _id(id) {}

					static Ptr create(size_t id) {
						return new TRemove(id);
					}

					Ptr apply(Object &obj) {
						log_assert(obj.type_id() == TYPE_ID, util::f("Expecting object of type UIDList but got ", obj.type_str()));

						UIDList<T> &cast_obj = obj;

						cast_obj._ignore_start();
						auto element = std::move(cast_obj[_id]);
						//TODO: remove to allow reusing ids
						cast_obj._ignore_end();

						return TInsert::create(_id, std::move(element));
					}

				private:
					size_t _id = 0;
			};

			class TInsert: public Transaction {
				public:
					TInsert(size_t id, T &&value): _id(id), _value(std::move(value)) {}

					static Ptr create(size_t id, T &&value) {
						return new TInsert(id, std::move(value));
					}

					Ptr apply(Object &obj) {
						log_assert(obj.type_id() == TYPE_ID, util::f("Expecting object of type UIDList but got ", obj.type_str()));
						UIDList<T> &cast_obj = obj;

						auto id = Container::IdTrait()(_value);

						cast_obj._ignore_start();
						cast_obj.insert(std::move(_value));
						cast_obj._ignore_end();

						return TRemove::create(id);
					}

				private:
					size_t _id;
					T _value;
			};

		public:
			using Container = ::util::UIDList<T>;
			using Element = Container::Element;
			using iterator = Container::iterator;
			using const_iterator = Container::const_iterator;

		public:
			uint32_t type_id() const { return TYPE_ID; }
			const char *type_str() const { return "UIDList"; }

			iterator begin() {
				return _list.begin();
			}
			iterator end() {
				return _list.end();
			}

			const_iterator begin() const {
				return _list.begin();
			}
			const_iterator end() const {
				return _list.end();
			}

			Element &operator[](uint32_t id) { return _list[id]; }
			Element const &operator[](uint32_t id) const { return _list[id]; }

			uint32_t get_id() const {
				return _list.get_id();
			}

			bool insert(T &&element) {
				_implicit_start();
				auto id = Container::IdTrait()(element);
				auto r = _list.insert(std::move(element));
				_implicit_end();

				_add_transaction(TRemove::create(id));

				return r;
			}

			void remove(uint32_t id) {
				_implicit_start();
				//TODO: make sure uidlist can resuse id
				auto e = std::move(_list[id]);
				_add_transaction(TInsert(id, std::move(e)));
				_implicit_end();
			}

		private:
			Container _list;
	};
}
