#include "CFG.hpp"

#include <sstream>
#include <string>

#include "tests/Test.hpp"

namespace cg {
	cfg::cfg(): _type(Type::none) { }

	cfg::cfg(cfg &&other) {
		_children = std::move(other._children);

		_ref = other._ref;
		other._ref = nullptr;

		_content = std::move(other._content);

		_type = other._type;
		other._type = Type::none;

		_name = std::move(other._name);
	}

	cfg &cfg::operator=(cfg &&other) {
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

	void cfg::destroy() {
		_children.clear();
		_ref = nullptr;
		_content.clear();
		_type = Type::none;
		_name.clear();
	}

	cfg cfg::dup() const {
		cfg result;
		for (auto &child : _children) {
			result._children.push_back(child.dup());
		}
		result._ref = _ref;
		result._content = _content;
		result._type = _type;
		result._name = _name;

		return result;
	}

	cfg cfg::literal(const std::string &str) {
		auto result = cfg();

		result._type = Type::literal;
		result._content = str;

		return result;
	}

	cfg cfg::ref(cfg const &other) {
		auto result = cfg();

		result._ref = &other;
		result._type = Type::reference;

		return result;
	}

	cfg cfg::seq(cfg &&lhs, cfg &&rhs) {
		auto result = cfg();

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

	cfg cfg::alt(cfg &&lhs, cfg &&rhs) {
		auto result = cfg();

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

	cfg cfg::cls(cfg const &c) {
		return cfg::cls(cfg::ref(c));
	}

	cfg cfg::cls(cfg &&c) {
		auto result = cfg();

		result._type = Type::closure;
		result._children.push_back(std::move(c));

		return result;
	}

	cfg cfg::opt(cfg const &c) {
		return cfg::opt(cfg::ref(c));
	}

	cfg cfg::opt(cfg &&c) {
		auto result = cfg();

		result._type = Type::optional;
		result._children.push_back(std::move(c));

		return result;
	}

	cfg::Container const &cfg::children() const {
		return _children;
	}
	std::string const &cfg::content() const {
		return _content;
	}
	cfg::Type cfg::type() const {
		return _type;
	}
	void cfg::set_name(std::string const &name) {
		_name = name;
	}
	cfg const &cfg::ref() const {
		return *_ref;
	}

	std::ostream &cfg::debug(std::ostream &os) const {
		bool first = true;
		switch (_type) {
			case Type::none:
				return os;
			case Type::literal:
				return os << "\"" << _content << "\"";
			case Type::reference:
				if (_ref->_name.empty()) {
					return os << "<ref>";
				} else {
					return os << _ref->_name;
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

	std::string cfg::str() const {
		std::stringstream ss;
		debug(ss);
		return ss.str();
	}

	TEST(cfg, literal) {
		auto literal_cfg = cfg::literal("simpler cfg");
		EXPECT_EQ(literal_cfg.str(), "\"simpler cfg\"");

		cfg literal_cfg2 = "another simpler cfg"_cfg;
		EXPECT_EQ(literal_cfg2.str(), "\"another simpler cfg\"");
	}

	TEST(cfg, ref) {
		auto anon_value = "anon"_cfg;

		auto named_value = "named"_cfg;
		named_value.set_name("named_ref");

		auto combined = anon_value + named_value;
		EXPECT_EQ(combined.str(), "<ref> + named_ref");
	}

	TEST(cfg, concat_str) {
		auto first = "f"_cfg + "ir"_cfg + "st"_cfg;
		EXPECT_EQ(first.str(), "\"first\"");

		auto second = cfg() + cfg::literal("second") + cfg();
		EXPECT_EQ(second.str(), "\"second\"");

		auto first_sec = first.dup() + second.dup();
		EXPECT_EQ(first_sec.str(), "\"firstsecond\"");

		auto first_third = cfg::seq(first.dup(), "third"_cfg);
		EXPECT_EQ(first_third.str(), "\"firstthird\"");
	}

	TEST(cfg, concat) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto third = "third"_cfg;
		third.set_name("third_ref");

		auto first_sec = first + second;
		EXPECT_EQ(first_sec.str(), "first_ref + second_ref");

		auto first_sec2 = first + second.dup();
		EXPECT_EQ(first_sec2.str(), "first_ref + \"second\"");

		auto first_sec3 = first.dup() + second;
		EXPECT_EQ(first_sec3.str(), "\"first\" + second_ref");

		auto first_sec_third = first + second + third;
		EXPECT_EQ(first_sec_third.str(), "first_ref + second_ref + third_ref");

		auto first_sec_third2 = first + second.dup() + third;
		EXPECT_EQ(first_sec_third2.str(), "first_ref + \"second\" + third_ref");

		auto first_sec_third3 = first.dup() + "_"_cfg + "_"_cfg + third;
		EXPECT_EQ(first_sec_third3.str(), "\"first__\" + third_ref");

	}

	TEST(cfg, alt) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto third = "third"_cfg;
		third.set_name("third_ref");

		auto first_sec = first | second;
		EXPECT_EQ(first_sec.str(), "first_ref | second_ref");

		auto first_sec2 = first.dup() | second;
		EXPECT_EQ(first_sec2.str(), "\"first\" | second_ref");

		auto first_sec3 = first | second.dup();
		EXPECT_EQ(first_sec3.str(), "first_ref | \"second\"");

		auto first_sec4 = first.dup() | second.dup();
		EXPECT_EQ(first_sec4.str(), "\"first\" | \"second\"");

		auto first_sec_third = first | second | third;
		EXPECT_EQ(first_sec_third.str(), "first_ref | second_ref | third_ref");

		auto first_sec_third2 = (first | second) | third;
		EXPECT_EQ(first_sec_third2.str(), "first_ref | second_ref | third_ref");

		auto first_sec_third3 = first | (second | third);
		EXPECT_EQ(first_sec_third3.str(), "first_ref | second_ref | third_ref");

		auto first_sec_third4 = (first | second.dup()) | third;
		EXPECT_EQ(first_sec_third4.str(), "first_ref | \"second\" | third_ref");
	}

	TEST(cfg, alt_concat) {
		auto first = "first"_cfg;
		first.set_name("first_ref");
		auto second = "second"_cfg;
		second.set_name("second_ref");
		auto third = "third"_cfg;
		third.set_name("third_ref");
		auto fourth = "fourth"_cfg;
		fourth.set_name("fourth_ref");

		auto test1 = first + second | third;
		EXPECT_EQ(test1.str(), "first_ref + second_ref | third_ref");

		auto test2 = (first + second) | third;
		EXPECT_EQ(test2.str(), "first_ref + second_ref | third_ref");

		auto test3 = first + (second | third);
		EXPECT_EQ(test3.str(), "first_ref + (second_ref | third_ref)");

		auto test4 = first.dup() + (second.dup() | third);
		EXPECT_EQ(test4.str(), "\"first\" + (\"second\" | third_ref)");

		auto test5 = first.dup() + second.dup() | third;
		EXPECT_EQ(test5.str(), "\"firstsecond\" | third_ref");

		auto test_long = (first + second) | (third + fourth);
		EXPECT_EQ(test_long.str(), "first_ref + second_ref | third_ref + fourth_ref");

		auto test_long2 = first + (second | third) + fourth;
		EXPECT_EQ(test_long2.str(), "first_ref + (second_ref | third_ref) + fourth_ref");

		cfg recurse_test;
		recurse_test = "end"_cfg | "e"_cfg + recurse_test;
		recurse_test.set_name("recurse_test");

		EXPECT_EQ(recurse_test.str(), "\"end\" | \"e\" + recurse_test");
	}

	TEST(cfg, closures) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto single_closure = cfg::cls(first);
		EXPECT_EQ(single_closure.str(), "[first_ref]");

		auto alt_closure = cfg::cls(first | second);
		EXPECT_EQ(alt_closure.str(), "[first_ref | second_ref]");

		auto alt_closure2 = cfg::cls(first.dup() | second.dup());
		EXPECT_EQ(alt_closure2.str(), "[\"first\" | \"second\"]");

		auto seq_closure = cfg::cls(first + second);
		EXPECT_EQ(seq_closure.str(), "[first_ref + second_ref]");

		auto seq_closure_combined = cfg::cls(first.dup() + second.dup());
		EXPECT_EQ(seq_closure_combined.str(), "[\"firstsecond\"]");
	}

	TEST(cfg, optional) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto third = "third"_cfg;
		third.set_name("third_ref");

		auto single = cfg::opt(first);
		EXPECT_EQ(single.str(), "(first_ref)?");

		auto single_literal = cfg::opt(first.dup());
		EXPECT_EQ(single_literal.str(), "(\"first\")?");

		auto concat_opt = cfg::opt(first + second) + third;
		EXPECT_EQ(concat_opt.str(), "(first_ref + second_ref)? + third_ref");

		auto combine_opt = cfg::opt(first.dup() + second.dup()) + third;
		EXPECT_EQ(combine_opt.str(), "(\"firstsecond\")? + third_ref");

		auto alt_opt = cfg::opt(first | second) | third;
		EXPECT_EQ(alt_opt.str(), "(first_ref | second_ref)? | third_ref");

		auto mixed_opt = first | cfg::opt(second + third);
		EXPECT_EQ(mixed_opt.str(), "first_ref | (second_ref + third_ref)?");

		auto mixed_opt2 = cfg::opt(second | third) + first;
		EXPECT_EQ(mixed_opt2.str(), "(second_ref | third_ref)? + first_ref");

		auto opt_nested = cfg::opt(cfg::opt(first.dup()));
		EXPECT_EQ(opt_nested.str(), "((\"first\")?)?");

	}
}


