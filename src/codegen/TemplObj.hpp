#pragma once

#include <map>
#include <variant>
#include <vector>
#include <string>
#include <functional>

#include "util/result.hpp"
#include "util/KError.hpp"

namespace cg {
	class TemplObj {
		public:
			using Number = double;
			using String = std::string;
			using List = std::vector<TemplObj>;
			using Dict = std::map<std::string, TemplObj>;
			using Callable = std::function<TemplObj(List)>;

			enum struct Type: size_t {
				String,
				List,
				Dict,
				Boolean,
				Integer,
				Callable,
				None
			};
		public:
			TemplObj() = default;
			TemplObj(TemplObj const &other) = default;
			TemplObj(TemplObj &&other) = default;
			TemplObj(String const &str);
			TemplObj(List const &list);
			TemplObj(Dict const &dict);
			TemplObj(bool val);
			TemplObj(int64_t val);
			TemplObj(int val);
			TemplObj(Callable callable);

			TemplObj(const char *str): _v(str) {}

			TemplObj& operator=(TemplObj const &other) = default;
			TemplObj& operator=(TemplObj &&other) = default;
			TemplObj& operator=(String const &str);
			TemplObj& operator=(List const &list);
			TemplObj& operator=(Dict const &dict);
			TemplObj& operator=(bool val);
			TemplObj& operator=(int64_t val);
			TemplObj& operator=(int val);
			TemplObj& operator=(Callable callable);

			TemplObj& operator=(const char *str);

			/**
			 * @brief Gets string representation
			 * @param[in] convert Whether type is automatically converted
			 */
			util::Result<std::string, KError> str(bool convert=true) const;

			Type type() const { return Type(_v.index()); }

			util::Result<List, KError> list() const;

			util::Result<bool, KError> boolean() const;

			util::Result<Dict, KError> dict() const;

			util::Result<Callable, KError> callable() const;

			util::Result<TemplObj, KError> get_attribute(std::string const &name) const;

		private:
			Dict _properties;

			std::variant<String, List, Dict, bool, int64_t, Callable> _v;
	};
}
