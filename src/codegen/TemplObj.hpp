#pragma once

#include <map>
#include <variant>
#include <vector>
#include <string>
#include <functional>

#include "util/result.hpp"
#include "util/KError.hpp"

namespace cg {
	class TemplObj;
	using TemplFloat = double;
	using TemplStr = std::string;
	using TemplList = std::vector<TemplObj>;
	using TemplDict = std::map<std::string, TemplObj>;
	using TemplBool = bool;
	using TemplInt = int64_t;

	using TemplFuncRes = util::Result<TemplObj, KError>;
	using TemplFunc = std::function<TemplFuncRes(TemplList)>;

	class TemplObj {
		public:

			enum struct Type: size_t {
				String,
				List,
				Dict,
				Boolean,
				Integer,
				Func,
				None
			};
		public:
			TemplObj() = default;
			TemplObj(TemplObj const &other) = default;
			TemplObj(TemplObj &&other) = default;
			TemplObj(TemplStr const &str);
			TemplObj(TemplList const &list);
			TemplObj(TemplDict const &dict);
			TemplObj(bool val);
			TemplObj(int64_t val);
			TemplObj(int val);
			TemplObj(TemplFunc const &func);
			/** @brief Catchall prob very dangerous so we'll see*/
			template<typename T> TemplObj(T const &v): _v(v) {}

			/**
			 * @brief Creates object or list depending on input
			 * Empty initializer_list: An empty object
			 * List of pairs with first element of string: An object
			 * Anything else: list
			 */
			TemplObj(std::initializer_list<TemplObj> args);
			TemplObj(const char *str): _v(str) {}

			TemplObj& operator=(TemplObj const &other) = default;
			TemplObj& operator=(TemplObj &&other) = default;
			TemplObj& operator=(TemplStr const &str);
			TemplObj& operator=(TemplList const &list);
			TemplObj& operator=(TemplDict const &dict);
			TemplObj& operator=(bool val);
			TemplObj& operator=(int64_t val);
			TemplObj& operator=(int val);
			TemplObj& operator=(TemplFunc const &func);

			TemplObj& operator=(const char *str);

			/**
			 * @brief Gets string representation
			 * @param[in] convert Whether type is automatically converted
			 */
			util::Result<std::string, KError> str(bool convert=true) const;

			Type type() const { return Type(_v.index()); }

			util::Result<TemplList, KError> list() const;

			util::Result<TemplBool, KError> boolean() const;

			util::Result<TemplInt, KError> integer() const;

			util::Result<TemplDict, KError> dict() const;

			util::Result<TemplFunc, KError> func() const;

			util::Result<TemplObj, KError> get_attribute(std::string const &name) const;

		private:
			TemplDict _properties;

			std::variant<TemplStr, TemplList, TemplDict, TemplBool, TemplInt, TemplFunc> _v;
	};

	/* Forgive me for what I have done */

	inline TemplFunc mk_templfunc(
		std::function<TemplFuncRes()> func
	) {
		return [&](TemplList l) -> TemplFuncRes {
			if (l.size() != 0) {
				return KError::codegen("More parameters expected");
			}
			return func();
		};
	}

	template<class ... Rest>
	inline TemplFunc mk_templfunc(
		std::function<TemplFuncRes(int64_t,  Rest...)> func
	) {
		return [&](TemplList l) -> TemplFuncRes {
			if (l.size() == 0) {
				return KError::codegen("Not enough arguments in function");
			}
			if (l[0].type() != TemplObj::Type::Integer) {
				return KError::codegen("Function expects arg type: int");
			}

			auto small_l = TemplList(l.begin()+1, l.end());
			auto small_func = [&](Rest...rest){ return func(l[0].integer().value(), rest...); };
			return mk_templfunc(std::function(small_func))(small_l);
		};
	}

	template<class ... Rest>
	inline  TemplFunc mk_templfunc(
		std::function<TemplFuncRes(TemplStr,  Rest...)> func
	) {
		return [&](TemplList l) -> TemplFuncRes {
			if (l.size() == 0) {
				return KError::codegen("Not enough arguments in function");
			}
			if (l[0].type() != TemplObj::Type::String) {
				return KError::codegen("Function expects arg type: str");
			}

			auto small_l = TemplList(l.begin()+1, l.end());
			auto small_func = [&](Rest...rest){ return func(l[0].str().value(), rest...); };
			return mk_templfunc(std::function(small_func))(small_l);
		};
	}

}
