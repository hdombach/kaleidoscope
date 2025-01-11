
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"
#include "SParser.hpp"
#include "AstNode.hpp"

#include <fstream>

namespace cg {
	TEST(ast_node, match_literals) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c["hello"] = "Hello"_cfg;

		EXPECT(c.prep());

		EXPECT_EQ(parser.match("Hello World", "hello").value(0), 5);
		EXPECT_EQ(parser.match("hello world", "hello").value(0), 0);
	}

	TEST(ast_node, match_number) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c["digit"] =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c["integer"] = c.cls(c.ref("digit"));
		c["decimal"] = c.ref("integer") + "."_cfg + c.ref("integer");

		EXPECT(c.prep());

		EXPECT_EQ(parser.match("145a", "digit").value(0), 1);
		EXPECT_EQ(parser.match("abc5", "digit").value(0), 0);

		EXPECT_EQ(parser.match("145a", "integer").value(0), 3);
		EXPECT_EQ(parser.match("abc5", "integer").value(0), 0);
		EXPECT_EQ(parser.match("91023", "integer").value(0), 5);

		EXPECT_EQ(parser.match("491f", "decimal").value(0), 0);
		EXPECT_EQ(parser.match("hello", "decimal").value(0), 0);
		EXPECT_EQ(parser.match("192.", "decimal").value(0), 4);
		EXPECT_EQ(parser.match(".89141", "decimal").value(0), 6);
		EXPECT_EQ(parser.match("..123", "decimal").value(0), 1);
		EXPECT_EQ(parser.match("15.9", "decimal").value(0), 4);
	}

	TEST(ast_node, match_math) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c["digit"] =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c["integer"] = c.ref("digit") + c.cls(c.ref("digit"));
		c["decimal"] = c.ref("integer") + c.opt("."_cfg + c.ref("integer"));
		c["exp_sing"] = c.ref("decimal");
		c["exp_inc_dec"] = c.cls("+"_cfg | "-"_cfg | "!"_cfg) + c.ref("exp_sing");
		c["exp_mult_div"] = c.ref("exp_inc_dec") + c.cls(("*"_cfg | "/"_cfg | "%"_cfg) + c.ref("exp_inc_dec"));
		c["exp_add_sub"] = c.ref("exp_mult_div") + c.cls(("+"_cfg | "-"_cfg) + c.ref("exp_mult_div"));
		c["exp"] = c.ref("exp_add_sub");

		EXPECT(c.prep());

		EXPECT_EQ(parser.match("1", "exp").value(0), 1);
		EXPECT_EQ(parser.match("42.1", "exp").value(0), 4);
		EXPECT_EQ(parser.match("192.12+41", "exp").value(0), 9);
		EXPECT_EQ(parser.match("192.12+41-0.12", "exp").value(0), 14);
		EXPECT_EQ(parser.match("19*1.0", "exp").value(0), 6);
		EXPECT_EQ(parser.match("19/1.0*20", "exp").value(0), 9);
		EXPECT_EQ(parser.match("5+2*12", "exp").value(0), 6);
		EXPECT_EQ(parser.match("5+-2*-+-12", "exp").value(0), 10);
	}

	TEST(ast_node, parse_match) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c["whitespace"] = c.cls(" "_cfg | "\t"_cfg | "\n"_cfg);
		c["digit"] =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c["integer"] = c.ref("digit") + c.cls(c.ref("digit"));
		c["decimal"] = c.ref("integer") + c.opt("."_cfg + c.ref("integer"));
		c["sing_exp"] = c.ref("whitespace") + c.ref("decimal");
		c["unary_exp"] = c.ref("sing_exp") | (c.opt(c.ref("whitespace") + ("+"_cfg | "-"_cfg | "!"_cfg)) + c.ref("unary_exp"));
		c["mult_div_exp"] = c.ref("unary_exp") + c.opt(c.ref("whitespace") + ("*"_cfg | "/"_cfg | "%"_cfg) + c.ref("mult_div_exp"));
		c["add_sub_exp"] = c.ref("mult_div_exp") + c.opt(c.ref("whitespace") + ("+"_cfg | "-"_cfg) + c.ref("add_sub_exp"));
		c["exp"] = c.ref("add_sub_exp");

		EXPECT(c.prep());

		auto prim_names = std::vector<std::string>{
			"whitespace",
			"sing_exp",
			"unary_exp",
			"mult_div_exp",
			"add_sub_exp"
		};

		EXPECT_EQ(
			parser.parse("1", "exp").value().compressed(prim_names).value().pre_order_str(),
			"exp add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		EXPECT_EQ(
			parser.parse("501.76", "exp").value().compressed(prim_names).value().pre_order_str(),
			"exp add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		EXPECT_EQ(
			parser.parse("41.2+14", "exp").value().compressed(prim_names).value().pre_order_str(),
			"exp add_sub_exp "
			"mult_div_exp unary_exp sing_exp "
			"add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		std::ofstream file("gen/ast_node_parse_math.gv", std::ios::out);
		parser
			.parse("4-3*82  /3+ 2.3", "exp").value()
			.compressed(prim_names).value()
			.debug_dot(file);
		file.close();

		EXPECT_EQ(
			parser
				.parse("4-3*82  /3+ 2.3", "exp").value()
				.compressed(prim_names).value()
				.pre_order_str(),
			"exp add_sub_exp "
				"mult_div_exp unary_exp sing_exp "
				"add_sub_exp "
					"mult_div_exp "
						"unary_exp sing_exp "
						"mult_div_exp "
							"unary_exp sing_exp "
							"whitespace "
							"mult_div_exp unary_exp sing_exp "
					"add_sub_exp mult_div_exp unary_exp sing_exp whitespace "
		);

		EXPECT_EQ(
			parser
				.parse("5+-2*-+-12", "exp").value()
				.compressed(prim_names).value()
				.pre_order_str(),
			"exp add_sub_exp "
				"mult_div_exp unary_exp sing_exp "
				"add_sub_exp mult_div_exp "
					"unary_exp unary_exp sing_exp "
					"mult_div_exp unary_exp unary_exp unary_exp unary_exp sing_exp "
		)
	}
}
