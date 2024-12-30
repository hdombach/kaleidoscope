#include "AST.hpp"
#include "tests/Test.hpp"

namespace cg {
	util::Result<size_t, ASTError> match_cfg_literal(const char *str, CFG const &cfg) {
		auto r = 0;
		for (auto c : cfg.content()) {
			if (str[r] != c) return ASTError{};
			r++;
		}
		return r;
	}

	util::Result<size_t, ASTError> match_cfg_ref(const char *str, CFG const &cfg) {
		return match_cfg(str, cfg.ref());
	}

	util::Result<size_t, ASTError> match_cfg_seq(const char *str, CFG const &cfg) {
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

	util::Result<size_t, ASTError> match_cfg_alt(const char *str, CFG const &cfg) {
		for (auto &child : cfg.children()) {
			if (auto i = match_cfg(str, child)) {
				return i;
			}
		}
		return ASTError{};
	}

	util::Result<size_t, ASTError> match_cfg_closure(const char *str, CFG const &cfg) {
		size_t r = 0;
		while (true) {
			if (auto i = match_cfg(str + r, cfg.children()[0])) {
				r += i.value();
			} else {
				return r;
			}
		}
	}

	util::Result<size_t, ASTError> match_cfg_opt(const char *str, CFG const &cfg) {
		return match_cfg(str, cfg.children()[0]).value(0);
	}

	util::Result<size_t, ASTError> match_cfg(const char *str, CFG const &cfg) {
		switch (cfg.type()) {
			case CFG::Type::none:
				return 0;
			case CFG::Type::literal:
				return match_cfg_literal(str, cfg);
			case CFG::Type::reference:
				return match_cfg_ref(str, cfg);
			case CFG::Type::sequence:
				return match_cfg_seq(str, cfg);
			case CFG::Type::alternative:
				return match_cfg_alt(str, cfg);
			case CFG::Type::closure:
				return match_cfg_closure(str, cfg);
			case CFG::Type::optional:
				return match_cfg_opt(str, cfg);
		}
	}

	TEST(ast, literals) {
		auto hello_world = "Hello World";
		auto hello_world_wrong = "hello world";

		auto hello_cfg = "Hello"_cfg;
		EXPECT_EQ(match_cfg(hello_world, hello_cfg).value(0), 5);

		EXPECT_EQ(match_cfg(hello_world_wrong, hello_cfg).value(0), 0);
	}

	TEST(ast, number) {
		auto digit =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;

		auto integer = CFG::cls(digit);

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

	TEST(ast, math) {
		auto digit =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;

		auto integer = digit + CFG::cls(digit);

		auto decimal = integer + CFG::opt("."_cfg + integer);

		auto exp_sing = CFG::ref(decimal);

		auto exp3 = CFG::cls("+"_cfg | "-"_cfg | "!"_cfg) + exp_sing;

		auto exp5 = exp3 + CFG::cls(("*"_cfg | "/"_cfg | "%"_cfg) + exp3);

		auto exp6 = exp5 + CFG::cls(("+"_cfg | "-"_cfg) + exp5);

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
}
