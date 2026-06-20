#pragma once

#include <vector>
#include "Object.hpp"
#include "Document.hpp"
#include "util/log.hpp"
#include "util/Util.hpp"

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
					TRemove(size_type idx): _idx(idx) {}

					static Ptr create(size_type idx) {
						return new TRemove(idx);
					}

					Ptr apply(Object &obj) {
						log_assert(obj.type_id() == TYPE_ID, util::f("Expecting object of type Vector but got ", obj.type_str()));
						Vector<T> &vobj = obj;

						vobj._ignore_start();
						auto element = std::move(vobj[_idx]);
						vobj.remove(_idx);
						vobj._ignore_end();

						return TInsert::create(_idx, std::move(element));
					}

				private:
					size_type _idx = 0;
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
}
