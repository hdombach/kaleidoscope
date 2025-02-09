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

	using FuncResult = util::Result<TemplObj, KError>;
	using Func = std::function<FuncResult(TemplList)>;

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
			TemplObj(Func const &func);
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
			TemplObj& operator=(Func const &func);

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

			util::Result<Func, KError> func() const;

			util::Result<TemplObj, KError> get_attribute(std::string const &name) const;

		private:
			TemplDict _properties;

			std::variant<TemplStr, TemplList, TemplDict, TemplBool, TemplInt, Func> _v;
	};
}
