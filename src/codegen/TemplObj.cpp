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
	TemplObj::TemplObj(TemplStr const &str) {
		_v = str;
	}

	TemplObj::TemplObj(TemplList const &list) {
		_v = list;
	}

	TemplObj::TemplObj(TemplDict const &dict) {
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

	TemplObj::TemplObj(TemplFunc const &func) {
		_v = func;
	}

	TemplObj::TemplObj(std::initializer_list<TemplObj> args) {
		auto dict = TemplDict();

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
					_v = TemplList(args);
					return;
				}
			}
		}
		_v = dict;
	}

	TemplObj& TemplObj::operator=(TemplStr const &str) {
		_v = str;
		return *this;
	}

	TemplObj& TemplObj::operator=(TemplList const &list) {
		_v = list;
		return *this;
	}

	TemplObj& TemplObj::operator=(TemplDict const &dict) {
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

	TemplObj& TemplObj::operator=(TemplFunc const &func) {
		_v = func;
		return *this;
	}

	TemplObj& TemplObj::operator=(const char *str) {
		_v = str;
		return *this;
	}

	util::Result<std::string, KError> TemplObj::str(bool convert) const {
		auto type = static_cast<Type>(_v.index());
		if (type == Type::String) {
			return std::get<TemplStr>(_v);
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
			case Type::Func:
				return {"<function>"};
			case Type::None:
				return {"<none>"};
		}
	}

	util::Result<TemplList, KError> TemplObj::list() const {
		if (type() == Type::List) {
			return std::get<TemplList>(_v);
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

	util::Result<TemplDict, KError> TemplObj::dict() const {
		if (type() == Type::Dict) {
			return std::get<TemplDict>(_v);
		} else {
			return KError::codegen("Object is not a dict");
		}
	}

	util::Result<TemplFunc, KError> TemplObj::func() const {
		if (type() == Type::Func) {
			return std::get<TemplFunc>(_v);
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
