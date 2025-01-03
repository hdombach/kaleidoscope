#include "CFG.hpp"

#include <sstream>
#include <string>

#include "tests/Test.hpp"

namespace cg {
	Cfg::Cfg(): _type(Type::none) { }

	Cfg::Cfg(Cfg &&other) {
		_children = std::move(other._children);

		_ref = other._ref;
		other._ref = nullptr;

		_content = std::move(other._content);

		_type = other._type;
		other._type = Type::none;

		_name = std::move(other._name);
	}

	Cfg &Cfg::operator=(Cfg &&other) {
		destroy();

		_children = std::move(other._children);

		_ref = other._ref;
		other._ref = nullptr;

		_content = std::move(other._content);

		_type = other._type;
		other._type = Type::none;

		_name = std::move(other._name);

		return *this;
	}

	void Cfg::destroy() {
		_children.clear();
		_ref = nullptr;
		_content.clear();
		_type = Type::none;
		_name.clear();
	}

	Cfg Cfg::dup() const {
		Cfg result;
		for (auto &child : _children) {
			result._children.push_back(child.dup());
		}
		result._ref = _ref;
		result._content = _content;
		result._type = _type;
		result._name = _name;

		return result;
	}

	Cfg Cfg::literal(const std::string &str) {
		auto result = Cfg();

		result._type = Type::literal;
		result._content = str;

		return result;
	}

	Cfg Cfg::ref(Cfg const &other) {
		auto result = Cfg();

		result._ref = &other;
		result._type = Type::reference;

		return result;
	}

	Cfg Cfg::seq(Cfg &&lhs, Cfg &&rhs) {
		auto result = Cfg();

		if (lhs.type() == Type::literal && rhs.type() == Type::literal) {
			result._type = Type::literal;
			result._content = lhs.content() + rhs.content();

			return result;
		}

		if (lhs.type() == Type::none) {
			return rhs;
		}
		if (rhs.type() == Type::none) {
			return lhs;
		}

		result._type = Type::sequence;

		if (lhs.type() == Type::sequence) {
			result._children = std::move(lhs._children);
		} else {
			result._children.push_back(std::move(lhs));
		}

		if (rhs.type() == Type::sequence) {
			for (auto &child : rhs._children) {
				result._children.push_back(std::move(child));
			}
		} else {
			result._children.push_back(std::move(rhs));
		}

		lhs.destroy();
		rhs.destroy();

		return result;
	}

	Cfg Cfg::alt(Cfg &&lhs, Cfg &&rhs) {
		auto result = Cfg();

		result._type = Type::alternative;

		if (lhs.type() == Type::none) {
			return rhs;
		}
		if (rhs.type() == Type::none) {
			return lhs;
		}

		if (lhs.type() == Type::alternative) {
			result._children = std::move(lhs._children);
		} else {
			result._children.push_back(std::move(lhs));
		}

		if (rhs.type() == Type::alternative) {
			for (auto &child : rhs._children) {
				result._children.push_back(std::move(child));
			}
		} else {
			result._children.push_back(std::move(rhs));
		}

		return result;
	}

	Cfg Cfg::cls(Cfg const &c) {
		return Cfg::cls(Cfg::ref(c));
	}

	Cfg Cfg::cls(Cfg &&c) {
		auto result = Cfg();

		result._type = Type::closure;
		result._children.push_back(std::move(c));

		return result;
	}

	Cfg Cfg::opt(Cfg const &c) {
		return Cfg::opt(Cfg::ref(c));
	}

	Cfg Cfg::opt(Cfg &&c) {
		auto result = Cfg();

		result._type = Type::optional;
		result._children.push_back(std::move(c));

		return result;
	}

	Cfg::Container const &Cfg::children() const {
		return _children;
	}
	std::string const &Cfg::content() const {
		return _content;
	}
	Cfg::Type Cfg::type() const {
		return _type;
	}
	void Cfg::set_name(std::string const &name) {
		_name = name;
	}
	Cfg const &Cfg::ref() const {
		return *_ref;
	}

	std::ostream &Cfg::debug(std::ostream &os) const {
		bool first = true;
		switch (_type) {
			case Type::none:
				return os;
			case Type::literal:
				return os << "\"" << _content << "\"";
			case Type::reference:
				if (_ref->_name.empty()) {
					return os << "<anon>";
				} else {
					return os << "<" << _ref->_name << ">";
				}
			case Type::sequence:
				for (auto &child : _children) {
					if (first) {
						first = false;
					} else {
						os << " + ";
					}

					if (child.type() == Type::alternative) {
						os << "(";
					}
					os << child;
					if (child.type() == Type::alternative) {
						os << ")";
					}
				}
				return os;
			case Type::alternative:
				for (auto &child : _children) {
					if (first) {
						first = false;
					} else {
						os << " | ";
					}

					os << child;
				}
				return os;
			case Type::closure:
				return os << "[" << _children[0] << "]";
			case Type::optional:
				return os << "(" << _children[0] << ")?";
		}
	}

	std::string Cfg::str() const {
		std::stringstream ss;
		debug(ss);
		return ss.str();
	}

}


