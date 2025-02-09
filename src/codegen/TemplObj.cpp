#include "TemplObj.hpp"

#include "util/log.hpp"
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

	TemplObj::TemplObj(Callable const &callable) {
		_v = callable;
	}

	TemplObj::TemplObj(std::initializer_list<TemplObj> args) {
		auto dict = TemplObj::Dict();

		if (args.size() == 0) {
			return;
		} else {
			for (auto const &arg : args) {
				auto b = arg.type() == Type::List &&
					(arg.list()->size() == 2 &&
					arg.list()->at(0).type() == Type::String);

				if (b) {
					auto l = arg.list().value(); // For some reason, using ref breaks things
					dict[l[0].str().value()] = l[1];
				} else {
					_v = List(args);
					return;
				}
			}
		}
		_v = dict;
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

	TemplObj& TemplObj::operator=(Callable const &callable) {
		_v = callable;
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
			case Type::Callable:
				return {"<callable>"};
			case Type::None:
				return {"<none>"};
		}
	}

	util::Result<TemplObj::List, KError> TemplObj::list() const {
		if (type() == Type::List) {
			return std::get<List>(_v);
		} else {
			return KError::codegen("Object is not a list");
		}
	}

	util::Result<bool, KError> TemplObj::boolean() const {
		if (type() == Type::Boolean) {
			return std::get<bool>(_v);
		} else {
			return KError::codegen("Object is not a boolean");
		}
	}

	util::Result<int64_t, KError> TemplObj::integer() const {
		if (type() == Type::Integer) {
			return std::get<int64_t>(_v);
		} else {
			return KError::codegen("Object is not an integer");
		}
	}

	util::Result<TemplObj::Dict, KError> TemplObj::dict() const {
		if (type() == Type::Dict) {
			return std::get<Dict>(_v);
		} else {
			return KError::codegen("Object is not a dict");
		}
	}

	util::Result<TemplObj::Callable, KError> TemplObj::callable() const {
		if (type() == Type::Callable) {
			return std::get<Callable>(_v);
		} else {
			return KError::codegen("Object is not a callable");
		}
	}

	util::Result<TemplObj, KError> TemplObj::get_attribute(std::string const &name) const {
		if (type() == Type::Dict) {
			if (dict()->contains(name)) {
				return dict()->at(name);
			} else {
				return KError::codegen("Property " + name + " not found.");
			}
		} else {
			return KError::internal("Default properties are not implimented yet");
		}
	}
}
