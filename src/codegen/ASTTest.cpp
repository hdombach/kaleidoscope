#include "AST.hpp"
#include "tests/Test.hpp"

namespace cg {
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
