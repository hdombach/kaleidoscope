#include "AST.hpp"
#include "tests/Test.hpp"
#include "CFG.hpp"

namespace cg {
	ASTNode::ASTNode():
		_cfg(nullptr),
		_size(0)
	{ }

	ASTNode::ASTNode(Cfg const &cfg) {
		_cfg = &cfg;
		_size = 0;
	}

	bool ASTNode::has_value() const {
		return _cfg;
	}

	void ASTNode::add_child(ASTNode const &node) {
		_children.push_back(node);
		/* Cache is invalidated */
		_size = 0;
	}

	void ASTNode::consume(char c) {
		_consumed.push_back(c);
		_size = 0;
	}

	size_t ASTNode::size() const {
		if (_size == 0) {
			_size = _calc_size();
		}
		return _size;
	}

	bool ASTNode::is_ref() const {
		return _cfg->type() == Cfg::Type::reference;
	}

	void ASTNode::compress() {
		auto new_children = std::vector<ASTNode>();
		for (auto &child : _children) {
			child.compress();
			_consumed += child._consumed;
			if (child.is_ref()) {
				new_children.push_back(child);
			}
		}
		_children = new_children;
	}

	size_t ASTNode::_calc_size() const {
		auto s = _consumed.size();
		for (auto &c : _children) {
			s += c.size();
		}
		return s;
	}

	util::Result<size_t, ASTError> match_cfg_literal(const char *str, Cfg const &cfg) {
		auto r = 0;
		for (auto c : cfg.content()) {
			if (str[r] != c) return ASTError{};
			r++;
		}
		return r;
	}

	util::Result<size_t, ASTError> match_cfg_ref(const char *str, Cfg const &cfg) {
		return match_cfg(str, cfg.ref());
	}

	util::Result<size_t, ASTError> match_cfg_seq(const char *str, Cfg const &cfg) {
		size_t r = 0;
		for (auto &child : cfg.children()) {
			if (auto i = match_cfg(str+r, child)) {
				r += i.value();
			} else {
				return ASTError{};
			}
		}
		return r;
	}

	util::Result<size_t, ASTError> match_cfg_alt(const char *str, Cfg const &cfg) {
		for (auto &child : cfg.children()) {
			if (auto i = match_cfg(str, child)) {
				return i;
			}
		}
		return ASTError{};
	}

	util::Result<size_t, ASTError> match_cfg_closure(const char *str, Cfg const &cfg) {
		size_t r = 0;
		while (true) {
			if (auto i = match_cfg(str + r, cfg.children()[0])) {
				r += i.value();
			} else {
				return r;
			}
		}
	}

	util::Result<size_t, ASTError> match_cfg_opt(const char *str, Cfg const &cfg) {
		return match_cfg(str, cfg.children()[0]).value(0);
	}

	util::Result<size_t, ASTError> match_cfg(const char *str, Cfg const &cfg) {
		switch (cfg.type()) {
			case Cfg::Type::none:
				return 0;
			case Cfg::Type::literal:
				return match_cfg_literal(str, cfg);
			case Cfg::Type::reference:
				return match_cfg_ref(str, cfg);
			case Cfg::Type::sequence:
				return match_cfg_seq(str, cfg);
			case Cfg::Type::alternative:
				return match_cfg_alt(str, cfg);
			case Cfg::Type::closure:
				return match_cfg_closure(str, cfg);
			case Cfg::Type::optional:
				return match_cfg_opt(str, cfg);
		}
	}

	/************************************************
	 *                  Parsing
	 ************************************************/

	util::Result<ASTNode, ASTError> parse_cfg_literal(
		const char *str,
		Cfg const &cfg
	) {
		size_t r = 0;
		auto node = ASTNode(cfg);
		for (auto c : cfg.content()) {
			if (str[r] != c) return ASTError{};
			node.consume(c);
			r++;
		}
		return {node};
	}

	util::Result<ASTNode, ASTError> parse_cfg_ref(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg);

		if (auto child = parse_cfg(str, cfg.ref())) {
			node.add_child(child.value());
		} else {
			return ASTError();
		}
		return node;
	}

	util::Result<ASTNode, ASTError> parse_cfg_seq(
		const char *str,
		Cfg const &cfg
	) {
		size_t r = 0;
		auto node = ASTNode(cfg);
		for (auto &child_cfg : cfg.children()) {
			if (auto child = parse_cfg(str+r, child_cfg)) {
				r += child.value().size();
				node.add_child(child.value());
			} else {
				return ASTError{};
			}
		}
		return node;
	}

	util::Result<ASTNode, ASTError> parse_cfg_alt(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg);
		for (auto &child_cfg : cfg.children()) {
			if (auto child = parse_cfg(str, child_cfg)) {
				node.add_child(child.value());
				return node;
			}
		}
		return ASTError{};
	}

	util::Result<ASTNode, ASTError> parse_cfg_closure(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg);
		size_t r = 0;
		while (true) {
			if (auto child = parse_cfg(str + r, cfg.children()[0])) {
				r += child.value().size();
				node.add_child(child.value());
			} else {
				return node;
			}
		}
	}

	util::Result<ASTNode, ASTError> parse_cfg_opt(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg);
		if (auto child = parse_cfg(str, cfg)) {
			node.add_child(child.value());
		}
		return node;
	}

	util::Result<ASTNode, ASTError> parse_cfg(const char *str, Cfg const &cfg) {
		switch (cfg.type()) {
			case Cfg::Type::none:
				return ASTError{};
			case Cfg::Type::literal:
				return parse_cfg_literal(str, cfg);
			case Cfg::Type::reference:
				return parse_cfg_ref(str, cfg);
			case Cfg::Type::sequence:
				return parse_cfg_seq(str, cfg);
			case Cfg::Type::alternative:
				return parse_cfg_alt(str, cfg);
			case Cfg::Type::closure:
				return parse_cfg_closure(str, cfg);
			case Cfg::Type::optional:
				return parse_cfg_opt(str, cfg);
		}
	}

	std::ostream &ASTNode::debug(std::ostream &os) const {
		bool is_first;
		os << "{";

		os << "\"cfg\": \"" << *_cfg << "\"";

		os << ", \"content\": " << _consumed;

		os << ", \"children\": [";
		is_first = true;
		for (auto &child : _children) {
			if (is_first) {
				is_first = false;
			} else {
				os << ", ";
			}

			os << child;
		}
		os << "]";

		os << "}";
		return os;
	}

	std::string ASTNode::str() const {
		std::stringstream ss;
		debug(ss);
		return ss.str();
	}

	TEST(ast, match_literals) {
		auto hello_world = "Hello World";
		auto hello_world_wrong = "hello world";

		auto hello_cfg = "Hello"_cfg;
		EXPECT_EQ(match_cfg(hello_world, hello_cfg).value(0), 5);

		EXPECT_EQ(match_cfg(hello_world_wrong, hello_cfg).value(0), 0);
	}

	TEST(ast, match_number) {
		auto digit =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;

		auto integer = Cfg::cls(digit);

		auto decimal = integer + "."_cfg + integer;

		EXPECT_EQ(match_cfg("145a", digit).value(0), 1);
		EXPECT_EQ(match_cfg("abc5", digit).value(0), 0);

		EXPECT_EQ(match_cfg("145a", integer).value(0), 3);
		EXPECT_EQ(match_cfg("abc5", integer).value(0), 0);
		EXPECT_EQ(match_cfg("91023", integer).value(0), 5);

		EXPECT_EQ(match_cfg("491f", decimal).value(0), 0);
		EXPECT_EQ(match_cfg("hello", decimal).value(0), 0);
		EXPECT_EQ(match_cfg("192.", decimal).value(0), 4);
		EXPECT_EQ(match_cfg(".89141", decimal).value(0), 6);
		EXPECT_EQ(match_cfg("..123", decimal).value(0), 1);

		EXPECT_EQ(match_cfg("15.9", decimal).value(0), 4);
	}

	TEST(ast, match_math) {
		auto digit =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;

		auto integer = digit + Cfg::cls(digit);

		auto decimal = integer + Cfg::opt("."_cfg + integer);

		auto exp_sing = Cfg::ref(decimal);

		auto exp3 = Cfg::cls("+"_cfg | "-"_cfg | "!"_cfg) + exp_sing;

		auto exp5 = exp3 + Cfg::cls(("*"_cfg | "/"_cfg | "%"_cfg) + exp3);

		auto exp6 = exp5 + Cfg::cls(("+"_cfg | "-"_cfg) + exp5);

		auto exp = std::ref(exp6);

		EXPECT_EQ(match_cfg("1", exp).value(0), 1);
		EXPECT_EQ(match_cfg("42.1", exp).value(0), 4);
		EXPECT_EQ(match_cfg("192.12+41", exp).value(0), 9);
		EXPECT_EQ(match_cfg("192.12+41-0.12", exp).value(0), 14);
		EXPECT_EQ(match_cfg("19*1.0", exp).value(0), 6);
		EXPECT_EQ(match_cfg("19/1.0*20", exp).value(0), 9);
		EXPECT_EQ(match_cfg("5+2*12", exp).value(0), 6);
		EXPECT_EQ(match_cfg("5+-2*-+-12", exp).value(0), 10);
	}

	TEST(ast, parse_math) {
		auto digit =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;

		auto integer = digit.dup() + Cfg::cls(digit.dup());

		auto decimal = integer.dup() + Cfg::opt("."_cfg + integer.dup());

		auto exp_sing = Cfg::ref(decimal.dup());

		auto exp3 = Cfg::cls("+"_cfg | "-"_cfg | "!"_cfg) + exp_sing;

		auto exp5 = exp3 + Cfg::cls(("*"_cfg | "/"_cfg | "%"_cfg) + exp3);

		auto exp6 = exp5 + Cfg::cls(("+"_cfg | "-"_cfg) + exp5);

		auto exp = exp6.dup();

		auto node = parse_cfg("1", exp);

		EXPECT_EQ(node.value().str(), "1");
	}
}
