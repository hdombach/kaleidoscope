#include "AST.hpp"
#include "tests/Test.hpp"
#include <fstream>
#include <ostream>

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
		Cfg exp3, exp5, exp6;
		auto whitespace = Cfg::cls(" "_cfg | "\t"_cfg | "\n"_cfg);
		auto digit =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		auto integer = digit + Cfg::cls(digit);
		auto decimal = integer + Cfg::opt("."_cfg + integer);
		auto exp_sing = whitespace + Cfg::ref(decimal);
		exp3 = exp_sing | (Cfg::opt(whitespace + ("+"_cfg | "-"_cfg | "!"_cfg)) + exp3);
		exp5 = exp3 + Cfg::opt(whitespace + ("*"_cfg | "/"_cfg | "%"_cfg) + exp5);
		exp6 = exp5 + Cfg::opt(whitespace + ("+"_cfg | "-"_cfg) + exp6);
		auto exp = Cfg::ref(exp6);

		whitespace.set_name("whitespace");
		digit.set_name("digit");
		integer.set_name("integer");
		decimal.set_name("decimal");
		exp_sing.set_name("single_exp");
		exp3.set_name("inc_dec_exp");
		exp5.set_name("mult_div_exp");
		exp6.set_name("add_sub_exp");
		exp.set_name("exp");

		auto prim = std::vector<Cfg const *>{
			&whitespace,
			&exp_sing,
			&exp3,
			&exp5,
			&exp6
		};


		EXPECT_EQ(
			parse_cfg("1", exp).value().compressed(prim).pre_order_str(),
			"exp add_sub_exp mult_div_exp inc_dec_exp single_exp "
		);

		EXPECT_EQ(
			parse_cfg("501.76", exp).value().compressed(prim).pre_order_str(),
			"exp add_sub_exp mult_div_exp inc_dec_exp single_exp "
		);

		EXPECT_EQ(
			parse_cfg("41.2+14", exp).value().compressed(prim).pre_order_str(),
			"exp add_sub_exp "
			"mult_div_exp inc_dec_exp single_exp "
			"add_sub_exp mult_div_exp inc_dec_exp single_exp "
		);

		EXPECT_EQ(
			parse_cfg("4-3*82  /3+ 2.3", exp).value().compressed(prim).pre_order_str(),
			"exp add_sub_exp "
				"mult_div_exp inc_dec_exp single_exp "
				"add_sub_exp "
					"mult_div_exp "
						"inc_dec_exp single_exp "
						"mult_div_exp "
							"inc_dec_exp single_exp "
							"whitespace "
							"mult_div_exp inc_dec_exp single_exp "
					"add_sub_exp mult_div_exp inc_dec_exp single_exp whitespace "
		);

		std::ofstream file("gen/ast_parse_math.gv", std::ios::out);
		parse_cfg("5+-2*-+-12", exp).value().compressed(prim).debug_dot(file);
		file.close();

		EXPECT_EQ(
			parse_cfg("5+-2*-+-12", exp).value().compressed(prim).pre_order_str(),
			"exp add_sub_exp "
				"mult_div_exp inc_dec_exp single_exp "
				"add_sub_exp mult_div_exp "
					"inc_dec_exp inc_dec_exp single_exp "
					"mult_div_exp inc_dec_exp inc_dec_exp inc_dec_exp inc_dec_exp single_exp "
		);
	}
}
