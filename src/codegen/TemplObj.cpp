#include "TemplObj.hpp"

#include <cctype>
#include <memory>

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
/*
	TemplObj& TemplObj::operator=(TemplStr const &str) {
		_v = str;
		_builtins = _str_builtins();
		return *this;
	}

	TemplObj& TemplObj::operator=(TemplList const &list) {
		_v = list;
		_builtins = _list_builtins();
		return *this;
	}

	TemplObj& TemplObj::operator=(TemplDict const &dict) {
		_v = dict;
		_builtins = nullptr;
		return *this;
	}

	TemplObj& TemplObj::operator=(bool val) {
		_v = val;
		_builtins = nullptr;
		return *this;
	}

	TemplObj& TemplObj::operator=(int64_t val) {
		_v = val;
		_builtins = nullptr;
		return *this;
	}

	TemplObj& TemplObj::operator=(int val) {
		_v = val;
		_builtins = nullptr;
		return *this;
	}

	TemplObj& TemplObj::operator=(TemplFunc const &func) {
		_v = func;
		_builtins = nullptr;
		return *this;
	}

	TemplObj& TemplObj::operator=(const char *str) {
		_v = str;
		_builtins = nullptr;
		return *this;
	}
*/
	TemplFuncRes TemplObj::unary_plus(TemplFuncRes const &val) {
		try {
			auto type = val->type();
			if (type == Type::Integer) {
				return {+val->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Invalid operation +",
					val->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::unary_min(TemplFuncRes const &val) {
		try {
			auto type = val->type();
			if (type == Type::Integer) {
				return {-val->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Invalid operation -",
					val->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::log_not(TemplFuncRes const &val) {
		try {
			auto type = val->type();
			if (type == Type::Boolean) {
				return {!val->boolean().value()};
			} else {
				return KError::codegen(util::f(
					"Invalid operation !",
					val->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::bit_not(TemplFuncRes const &val) {
		try {
			auto type = val->type();
			if (type == Type::Integer) {
				return {~val->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Invalid operation ~",
					val->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::mult(TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		try {
			auto type = lhs->type();
			if (type == Type::Integer) {
				return {lhs->integer().value() * rhs->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Invalid operation ",
					lhs->type_str(),
					" * ",
					rhs->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::div(TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		try {
			auto type = lhs->type();
			if (type == Type::Integer) {
				return {lhs->integer().value() / rhs->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Invalid operation ",
					lhs->type_str(),
					" / ",
					rhs->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::mod(TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		try {
			auto type = lhs->type();
			if (type == Type::Integer) {
				return {lhs->integer().value() % rhs->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Invalid operation ",
					lhs->type_str(),
					" % ",
					rhs->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::add( TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		try {
			auto type = lhs->type();
			if (type == Type::Integer) {
				return {lhs->integer().value() + rhs->integer().value()};
			} else if (type == Type::String) {
				return {lhs->str().value() + rhs->str().value()};
			} else {
				//TODO: add string conantination
				return KError::codegen(util::f(
					"Invalid operation ",
					lhs->type_str(),
					" + ",
					rhs->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::sub( TemplFuncRes const &lhs, TemplFuncRes const &rhs) {
		try {
			auto type = lhs->type();
			if (type == Type::Integer) {
				return {lhs->integer().value() - rhs->integer().value()};
			} else {
				//TODO: add string conantination
				return KError::codegen(util::f(
					"Cannot do operator ",
					lhs->type_str(),
					" - ",
					rhs->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::bitshift_l(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() << r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" << ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::bitshift_r(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() >> r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" >> ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::comp_g(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() > r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" > ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::comp_ge(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() >= r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" >= ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::comp_l(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() < r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" < ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::comp_le(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() <= r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" <= ",
					r->type_str()
				));
			} 
		} catch_kerror;
	}

	TemplFuncRes TemplObj::comp_eq(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() == r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" == ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::comp_neq(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() != r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" != ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::bit_and(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() & r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" & ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::bit_xor(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() ^ r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" ^ ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::bit_or(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Integer) {
				return {l->integer().value() | r->integer().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" | ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::log_and(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Boolean) {
				return {l->boolean().value() && r->boolean().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" && ",
					r->type_str()
				));
			}
		} catch_kerror;
	}

	TemplFuncRes TemplObj::log_or(TemplFuncRes const &l, TemplFuncRes const &r) {
		try {
			auto type = l->type();
			if (type == Type::Boolean) {
				return {l->boolean().value() || r->boolean().value()};
			} else {
				return KError::codegen(util::f(
					"Cannot do operator ",
					l->type_str(),
					" || ",
					r->type_str()
				));
			}
		} catch_kerror;
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
		if (_builtins && _builtins->contains(name)) {
			auto builtin = _builtins->at(name);
			if (builtin.type() == Type::Func) {
				return {builtin.func().value()};
			}
			return _builtins->at(name);
		}
		if (type() == Type::Dict) {
			if (dict()->contains(name)) {
				return dict()->at(name);
			} else {
				return KError::codegen("Property " + name + " not found.");
			}
		} else {
			return KError::internal(
				"Default property " + name + " does not exist for type " + type_str(),
				location(util::FileLocation())
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

	TemplDict *TemplObj::_list_builtins() {
		static auto properties = TemplDict();

		if (properties.size() == 0) {
			properties = TemplDict{
				{"length", mk_templfunc(_list_length)},
				{"empty", mk_templfunc(_list_empty)}
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
