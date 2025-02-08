#include "TemplObj.hpp"

#include "util/result.hpp"
#include "util/KError.hpp"

//https://en.cppreference.com/w/cpp/utility/variant/visit2
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };

template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

namespace cg {
	TemplObj::TemplObj(String const &str) {
		_v = str;
	}

	TemplObj::TemplObj(List const &list) {
		_v = list;
	}

	TemplObj::TemplObj(Dict const &dict) {
		_v = dict;
	}

	TemplObj::TemplObj(bool val) {
		_v = val;
	}

	TemplObj::TemplObj(int64_t val) {
		_v = val;
	}

	TemplObj::TemplObj(int val) {
		_v = val;
	}

	TemplObj& TemplObj::operator=(String const &str) {
		_v = str;
		return *this;
	}

	TemplObj& TemplObj::operator=(List const &list) {
		_v = list;
		return *this;
	}

	TemplObj& TemplObj::operator=(Dict const &dict) {
		_v = dict;
		return *this;
	}

	TemplObj& TemplObj::operator=(bool val) {
		_v = val;
		return *this;
	}

	TemplObj& TemplObj::operator=(int64_t val) {
		_v = val;
		return *this;
	}

	TemplObj& TemplObj::operator=(int val) {
		_v = val;
		return *this;
	}

	TemplObj& TemplObj::operator=(const char *str) {
		_v = str;
		return *this;
	}

	util::Result<std::string, KError> TemplObj::str(bool convert) const {
		auto type = static_cast<Type>(_v.index());
		if (type == Type::String) {
			return std::get<String>(_v);
		}
		if (!convert) {
			return KError::codegen("Unknown conversion to str");
		}

		switch (type) {
			case Type::String:
				return {""}; // should be impossible
			case Type::List:
				return {"<list>"};
			case Type::Dict:
				return {"<dict>"};
			case Type::Boolean:
				return {std::get<bool>(_v) ? "<true>" : "<false>"};
			case Type::Integer:
				return {std::to_string(std::get<int64_t>(_v))};
		}
	}

	util::Result<TemplObj, KError> TemplObj::get_attribute(std::string const &name) {
		if (type() == Type::Dict) {
			if (dict().contains(name)) {
				return dict().at(name);
			} else {
				return KError::codegen("Property " + name + " not found.");
			}
		} else {
			return KError::internal("not implimented yet");
		}
	}
}
