#include "TemplObj.hpp"

#include <cctype>
#include <variant>

#include "util/format.hpp"
#include "util/result.hpp"

using util::f;

//https://en.cppreference.com/w/cpp/utility/variant/visit2
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };

template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

namespace cg {
	TemplObj::TemplObj(TemplStr const &str) {
		_v = str;
		_builtins = _str_builtins();
	}

	TemplObj::TemplObj(TemplList const &list) {
		_v = list;
		_builtins = _list_builtins();
	}

	TemplObj::TemplObj(TemplDict const &dict) {
		_v = dict;
		_builtins = nullptr;
	}

	TemplObj::TemplObj(bool val) {
		_v = val;
		_builtins = nullptr;
	}

	TemplObj::TemplObj(int64_t val) {
		_v = val;
		_builtins = nullptr;
	}

	TemplObj::TemplObj(int val) {
		_v = val;
		_builtins = nullptr;
	}

	TemplObj::TemplObj(uint32_t val) {
		_v = val;
		_builtins = nullptr;
	}

	TemplObj::TemplObj(TemplFunc const &func) {
		_v = func;
		_builtins = nullptr;
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
					_builtins = _list_builtins();
					return;
				}
			}
		}
		_v = dict;
		_builtins = nullptr;
	}

	TemplObj::TemplObj(const char *str): _v(str), _builtins(_str_builtins()) {}

	TemplFuncRes TemplObj::unary_plus(TemplFuncRes const &val) {
		auto type = val->type();
		if (type == Type::Integer) {
			return {+val->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Invalid operation + ", val->type_str()));
		}
	}

	TemplFuncRes TemplObj::unary_min(TemplFuncRes const &val) {
		auto type = val->type();
		if (type == Type::Integer) {
			return {-val->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Invalid operationr - ", val->type_str()));
		}
	}

	TemplFuncRes TemplObj::log_not(TemplFuncRes const &val) {
		auto type = val->type();
		if (type == Type::Boolean) {
			return {!val->boolean().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Invalid operation !", val->type_str()));
		}
	}

	TemplFuncRes TemplObj::mult(TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		auto ltype = lhs->type();
		auto rtype = rhs->type();
		if (ltype != rtype || ltype != Type::Integer) {
			return Error(ErrorType::RUNTIME_CG, f("invalid operation ", lhs->type_str(), " * ", rhs->type_str()));
		}
		return {lhs->integer().value() * rhs->integer().value()};
	}

	TemplFuncRes TemplObj::div(TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		auto ltype = lhs->type();
		auto rtype = rhs->type();
		if (ltype != rtype || ltype != Type::Integer) {
			return Error(ErrorType::RUNTIME_CG, f("Invalid operation ", lhs->type_str(), " / ", rhs->type_str()));
		}
		return {lhs->integer().value() / rhs->integer().value()};
	}

	TemplFuncRes TemplObj::mod(TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		auto ltype = lhs->type();
		auto rtype = rhs->type();
		if (ltype != rtype || ltype != Type::Integer) {
			return Error(ErrorType::RUNTIME_CG, f("Invalid operationr ", lhs->type_str(), " % ", rhs->type_str()));
		}
		return {lhs->integer().value() % rhs->integer().value()};
	}

	TemplFuncRes TemplObj::add( TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		auto ltype = lhs->type();
		auto rtype = rhs->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the addition operator must match");
		}
		if (ltype == Type::Integer) {
			return {lhs->integer().value() + rhs->integer().value()};
		} else if (ltype == Type::String) {
			return {lhs->str().value() + rhs->str().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Invalid operation ", lhs->type_str(), " + ", rhs->type_str()));
		}
	}

	TemplFuncRes TemplObj::sub( TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		auto ltype = lhs->type();
		auto rtype = rhs->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the subtraction operator must match");
		}
		if (ltype == Type::Integer) {
			return {lhs->integer().value() - rhs->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", lhs->type_str(), " - ", rhs->type_str()));
		}
	}

	TemplFuncRes TemplObj::comp_g(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the greater than operator must match");
		}
		if (ltype == Type::Integer) {
			return {l->integer().value() > r->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Canot do operator ", l->type_str(), " > ", r->type_str()));
		}
	}

	TemplFuncRes TemplObj::comp_ge(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the greater than or equal operator must match");
		}
		if (ltype == Type::Integer) {
			return {l->integer().value() >= r->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", l->type_str(), " >= ", r->type_str()));
		}
	}

	TemplFuncRes TemplObj::comp_l(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the less than operator must match");
		}
		if (ltype == Type::Integer) {
			return {l->integer().value() < r->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", l->type_str(), " < ", r->type_str()));
		}
	}

	TemplFuncRes TemplObj::comp_le(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the less than or equal operator must match");
		}
		if (ltype == Type::Integer) {
			return {l->integer().value() <= r->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", l->type_str(), " <= ", r->type_str()));
		}
	}

	TemplFuncRes TemplObj::comp_eq(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the equality operator must match");
		}
		if (ltype == Type::Integer) {
			return {l->integer().value() == r->integer().value()};
		} else if (ltype == Type::String) {
			return {l->str(false).value() == r->str(false).value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", l->type_str(), " == ", r->type_str()));
		}
	}

	TemplFuncRes TemplObj::comp_neq(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the not equal operator must match");
		}
		if (ltype == Type::Integer) {
			return {l->integer().value() != r->integer().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", l->type_str(), " != ", r->type_str()));
		}
	}

	TemplFuncRes TemplObj::log_and(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the and operator must match");
		}
		if (ltype == Type::Boolean) {
			return {l->boolean().value() && r->boolean().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", l->type_str(), " && ", r->type_str()));
		}
	}

	TemplFuncRes TemplObj::log_or(TemplFuncRes const &l, TemplFuncRes const &r) {
		auto ltype = l->type();
		auto rtype = r->type();
		if (ltype != rtype) {
			return Error(ErrorType::RUNTIME_CG, "Left and right hand side of the or operator must match");
		}
		if (ltype == Type::Boolean) {
			return {l->boolean().value() || r->boolean().value()};
		} else {
			return Error(ErrorType::RUNTIME_CG, f("Cannot do operator ", l->type_str(), " || ", r->type_str()));
		}
	}

	util::Result<std::string, Error> TemplObj::str(bool convert) const {
		auto type = static_cast<Type>(_v.index());
		if (type == Type::String) {
			CG_ASSERT(std::holds_alternative<TemplStr>(_v), "Invalid internal state: expecting str");
			return std::get<TemplStr>(_v);
		}
		if (!convert) {
			return Error(ErrorType::RUNTIME_CG, "Unknown conversion to str");
		}

		switch (type) {
			case Type::String:
				return {""}; // should be impossible
			case Type::List:
				return {"<list>"};
			case Type::Dict:
				return {"<dict>"};
			case Type::Boolean:
				CG_ASSERT(std::holds_alternative<TemplBool>(_v), "Invalid internal state: expecting bool");
				return {std::get<TemplBool>(_v) ? "<true>" : "<false>"};
			case Type::Integer:
				CG_ASSERT(std::holds_alternative<TemplInt>(_v), "Invalid internal state: expecting int64");
				return {std::to_string(std::get<TemplInt>(_v))};
			case Type::Func:
				return {"<function>"};
			case Type::None:
				return {"<none>"};
		}
	}

	TemplObj::Type TemplObj::type() const {
		return Type(_v.index());
	}

	const char *TemplObj::type_str() const {
		return std::vector<const char *>{
			"String",
			"List",
			"Dict",
			"Boolean",
			"Integer",
			"Func",
			"None"
		}[static_cast<size_t>(type())];
	}

	util::Result<TemplList, Error> TemplObj::list() const {
		if (type() == Type::List) {
			CG_ASSERT(std::holds_alternative<TemplList>(_v), "Invalid internal state: expecting TemplList");
			return std::get<TemplList>(_v);
		} else {
			return Error(ErrorType::RUNTIME_CG, "Object is not a list", location({}));
		}
	}

	util::Result<TemplBool, Error> TemplObj::boolean() const {
		if (type() == Type::Boolean) {
			CG_ASSERT(std::holds_alternative<TemplBool>(_v), "Invalid internal state: expecting TemplBool");
			return std::get<bool>(_v);
		} else {
			return Error(ErrorType::RUNTIME_CG, "Object is not a boolean", location({}));
		}
	}

	util::Result<TemplInt, Error> TemplObj::integer() const {
		if (type() == Type::Integer) {
			CG_ASSERT(std::holds_alternative<TemplInt>(_v), "Invalid internal state: expecting TemplInt");
			return std::get<TemplInt>(_v);
		} else {
			return Error(ErrorType::RUNTIME_CG, "Object is not an integer", location({}));
		}
	}

	util::Result<TemplDict, Error> TemplObj::dict() const {
		if (type() == Type::Dict) {
			CG_ASSERT(std::holds_alternative<TemplDict>(_v), "Invalid internal state: expecting dict");
			return std::get<TemplDict>(_v);
		} else {
			return Error(ErrorType::RUNTIME_CG, "Object is not a dict", location({}));
		}
	}

	util::Result<TemplFunc, Error> TemplObj::func() const {
		if (type() == Type::Func) {
			CG_ASSERT(std::holds_alternative<TemplFunc>(_v), "Invalid internal state: expecting func");
			return std::get<TemplFunc>(_v);
		} else {
			return Error(ErrorType::RUNTIME_CG, "Object is not a callable", location({}));
		}
	}

	util::Result<TemplObj, Error> TemplObj::get_attribute(std::string const &name) const {
		if (_builtins && _builtins->contains(name)) {
			auto builtin = _builtins->at(name);
			if (builtin.type() == Type::Func) {
				return {builtin.func().value()};
			}
			return _builtins->at(name);
		}
		if (type() == Type::Dict) {
			if (dict()->contains(name)) { return dict()->at(name);
			} else {
				return Error(ErrorType::RUNTIME_CG, f("Property ", name, " not found."));
			}
		} else {
			return Error(
				ErrorType::RUNTIME_CG,
				f("Default property ", name, " does not exist for type ", type_str()),
				location({})
			);
		}
	}

	TemplObj &TemplObj::set_location(util::FileLocation const &location) {
		_location = location;
		return *this;
	}

	util::FileLocation TemplObj::location(
		util::FileLocation const &default_location
	) const {
		if (_location) {
			return _location.value();
		} else {
			return default_location;
		}
	}

	/** Builtin properties */
	TemplFuncRes _list_length(TemplList l) {
		return {TemplInt(l.size())};
	}

	TemplFuncRes _list_empty(TemplList l) {
		return {TemplBool(l.empty())};
	}

	TemplFuncRes _list_index(TemplList l, TemplObj element) {
		int i = 0;
		for (auto &e : l) {
			if ((e == element)->boolean().value()) return {i};
			i++;
		}
		return Error(ErrorType::RUNTIME_CG, f("Element ", element.str().value(), " not found in list"));
	}

	TemplDict *TemplObj::_list_builtins() {
		static auto properties = TemplDict();

		if (properties.size() == 0) {
			properties = TemplDict{
				{"length", mk_templfunc(_list_length)},
				{"empty", mk_templfunc(_list_empty)},
				{"index", mk_templfunc(_list_index)}
			};
		}

		return &properties;
	}

	TemplFuncRes _str_length(TemplStr s) {
		return {TemplInt(s.size())};
	}

	TemplFuncRes _str_empty(TemplStr s) {
		return {TemplBool(s.empty())};
	}

	TemplFuncRes _str_lower(TemplStr s) {
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return {s};
	}

	TemplFuncRes _str_upper(TemplStr s) {
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		return {s};
	}

	TemplDict *TemplObj::_str_builtins() {
		static auto properties = TemplDict();

		if (properties.size() == 0) {
			properties = TemplDict{
				{"length", mk_templfunc(_str_length)},
				{"empty", mk_templfunc(_str_empty)},
				{"upper", mk_templfunc(_str_upper)},
				{"lower", mk_templfunc(_str_lower)},
			};
		}

		return &properties;
	}
}
