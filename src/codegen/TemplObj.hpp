#pragma once

#include <map>
#include <variant>
#include <vector>
#include <string>
#include <functional>

#include "util/result.hpp"
#include "util/errors.hpp"

namespace cg {
	class TemplObj {
		public:
			using Number = double;
			using String = std::string;
			using List = std::vector<TemplObj>;
			using Dict = std::map<std::string, TemplObj>;
			using Callable = std::function<void(List)>;

			enum struct Type: size_t {
				String,
				List,
				Dict
			};
		public:
			TemplObj() = default;
			TemplObj(TemplObj const &other) = default;
			TemplObj(TemplObj &&other) = default;
			TemplObj(String const &str): _v(str) {}
			TemplObj(List const &list): _v(list) {}
			TemplObj(Dict const &dict): _v(dict) {}

			TemplObj& operator=(TemplObj const &other) = default;
			TemplObj& operator=(TemplObj &&other) = default;
			TemplObj& operator=(String const &str) { _v = str; return *this; }
			TemplObj& operator=(List const &list) { _v = list; return *this; }
			TemplObj& operator=(Dict const &dict) { _v = dict; return *this; }

			/**
			 * @brief Gets string representation
			 * @param[in] convert Whether type is automatically converted
			 */
			util::Result<std::string, KError> str(bool convert=true) const;

			Type type() const;

		private:
			std::variant<String, List, Dict> _v;
	};
}
