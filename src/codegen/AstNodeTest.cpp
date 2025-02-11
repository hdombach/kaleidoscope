
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"
#include "SParser.hpp"
#include "AstNode.hpp"

#include <fstream>

namespace cg {
	TEST(ast_node, match_literals) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c.prim("hello") = "Hello"_cfg;

		EXPECT(c.prep());

		EXPECT_EQ(parser.match("Hello", "hello").value(), 5);
		EXPECT_KERROR(parser.match("hello", "hello"), KError::Type::CODEGEN);
	}

	TEST(ast_node, match_number) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c.prim("digit") =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c.prim("integer") = c.cls(c.ref("digit"));
		c.prim("decimal") = c.ref("integer") + "."_cfg + c.ref("integer");

		EXPECT(c.prep());

		EXPECT_EQ(parser.match("1", "digit").value(), 1);
		EXPECT_KERROR(parser.match("abc5", "digit"), KError::Type::CODEGEN);

		EXPECT_KERROR(parser.match("145a", "integer"), KError::Type::CODEGEN);
		EXPECT_EQ(parser.match("145", "integer").value(), 3);
		EXPECT_KERROR(parser.match("abc5", "integer"), KError::Type::CODEGEN);
		EXPECT_EQ(parser.match("91023", "integer").value(), 5);

		EXPECT_KERROR(parser.match("491f", "decimal"), KError::Type::CODEGEN);
		EXPECT_KERROR(parser.match("hello", "decimal"), KError::Type::CODEGEN);
		EXPECT_EQ(parser.match("192.", "decimal").value(), 4);
		EXPECT_EQ(parser.match(".89141", "decimal").value(), 6);
		EXPECT_KERROR(parser.match("..123", "decimal"), KError::Type::CODEGEN);
		EXPECT_EQ(parser.match(".", "decimal").value(), 1);
		EXPECT_EQ(parser.match("15.9", "decimal").value(), 4);
	}

	TEST(ast_node, match_math) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c.prim("digit") =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c.prim("integer") = c.ref("digit") + c.cls(c.ref("digit"));
		c.prim("decimal") = c.ref("integer") + c.opt("."_cfg + c.ref("integer"));
		c.prim("exp_sing") = c.ref("decimal");
		c.prim("exp_inc_dec") = c.cls("+"_cfg | "-"_cfg | "!"_cfg) + c.ref("exp_sing");
		c.prim("exp_mult_div") = c.ref("exp_inc_dec") + c.cls(("*"_cfg | "/"_cfg | "%"_cfg) + c.ref("exp_inc_dec"));
		c.prim("exp_add_sub") = c.ref("exp_mult_div") + c.cls(("+"_cfg | "-"_cfg) + c.ref("exp_mult_div"));
		c.prim("exp") = c.ref("exp_add_sub");

		EXPECT(c.prep());

		EXPECT_EQ(parser.match("1", "exp").value(), 1);
		EXPECT_EQ(parser.match("42.1", "exp").value(), 4);
		EXPECT_EQ(parser.match("192.12+41", "exp").value(), 9);
		EXPECT_EQ(parser.match("192.12+41-0.12", "exp").value(), 14);
		EXPECT_EQ(parser.match("19*1.0", "exp").value(), 6);
		EXPECT_EQ(parser.match("19/1.0*20", "exp").value(), 9);
		EXPECT_EQ(parser.match("5+2*12", "exp").value(), 6);
		EXPECT_EQ(parser.match("5+-2*-+-12", "exp").value(), 10);
	}

	TEST(ast_node, match_neg) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c.prim("not_a") = c.cls(!"a"_cfg);
		c.prim("consonants") = c.cls(!("a"_cfg | "e"_cfg | "i"_cfg | "o"_cfg | "u"_cfg | "y"_cfg));
		c.prim("whitespace") = c.cls(" "_cfg | "\t"_cfg | "\n"_cfg);
		c.prim("mixed") = c.cls(!("if"_cfg + c.ref("whitespace") + ("true"_cfg | "false"_cfg)));

		EXPECT(c.prep());

		EXPECT_KERROR(parser.match("abcd", "not_a"), KError::Type::CODEGEN);
		EXPECT_KERROR(parser.match("change", "not_a"), KError::Type::CODEGEN);
		EXPECT_EQ(parser.match("ch", "not_a").value(), 2);
		EXPECT_KERROR(parser.match("mndups", "consonants"), KError::Type::CODEGEN);
		EXPECT_EQ(parser.match("mnd", "consonants").value(), 3);
		EXPECT_KERROR(parser.match("if flse if  false", "mixed"), KError::Type::CODEGEN);
		EXPECT_EQ(parser.match("if flse ", "mixed").value(), 8);
	}

	TEST(ast_node, parse_match) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c.prim("whitespace") = c.cls(" "_cfg | "\t"_cfg | "\n"_cfg);
		c.temp("digit") =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c.temp("integer") = c.ref("digit") + c.cls(c.ref("digit"));
		c.temp("decimal") = c.ref("integer") + c.opt("."_cfg + c.ref("integer"));
		c.prim("sing_exp") = c.ref("whitespace") + c.ref("decimal");
		c.prim("unary_exp") = c.ref("sing_exp") | (c.opt(c.ref("whitespace") + ("+"_cfg | "-"_cfg | "!"_cfg)) + c.ref("unary_exp"));
		c.prim("mult_div_exp") = c.ref("unary_exp") + c.opt(c.ref("whitespace") + ("*"_cfg | "/"_cfg | "%"_cfg) + c.ref("mult_div_exp"));
		c.prim("add_sub_exp") = c.ref("mult_div_exp") + c.opt(c.ref("whitespace") + ("+"_cfg | "-"_cfg) + c.ref("add_sub_exp"));
		c.prim("exp") = c.ref("add_sub_exp");

		EXPECT(c.prep());

		EXPECT_EQ(
			parser.parse("1", "exp")->compressed()->pre_order_str(),
			"exp add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		EXPECT_EQ(
			parser.parse("501.76", "exp")->compressed()->pre_order_str(),
			"exp add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		EXPECT_EQ(
			parser.parse("41.2+14", "exp")->compressed()->pre_order_str(),
			"exp add_sub_exp "
			"mult_div_exp unary_exp sing_exp "
			"add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		std::ofstream file("gen/ast_node_parse_math.gv");
		parser.parse("4-3*82  /3+ 2.3", "exp")->compressed()->debug_dot(file);
		file.close();

		EXPECT_EQ(
			parser.parse("4-3*82  /3+ 2.3", "exp")->compressed()->pre_order_str(),
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
			parser.parse("5+-2*-+-12", "exp") ->compressed() ->pre_order_str(),
			"exp add_sub_exp "
				"mult_div_exp unary_exp sing_exp "
				"add_sub_exp mult_div_exp "
					"unary_exp unary_exp sing_exp "
					"mult_div_exp unary_exp unary_exp unary_exp unary_exp sing_exp "
		);
	}

	TEST(ast_node, parse_neg) {
		auto c = CfgContext();
		auto parser = SParser(c);

		c.prim("whitespace") = c.cls(" "_cfg | "\t"_cfg | "\n"_cfg);

		c.prim("beg_exp") = "{{"_cfg;
		c.prim("end_exp") = "}}"_cfg;
		c.prim("beg_cmt") = "{#"_cfg;
		c.prim("end_cmt") = "#}"_cfg;

		c.temp("digit") =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c.prim("integer") = c.ref("digit") + c.cls(c.ref("digit"));

		c.prim("comment") = c.ref("beg_cmt") + c.cls(!c.ref("end_cmt")) + c.ref("end_cmt");
		c.prim("exp") = c.ref("beg_exp") + c.ref("whitespace") + c.ref("integer") + c.ref("whitespace") + c.ref("end_exp");

		c.prim("raw") = c.cls(!(c.ref("beg_exp") | c.ref("beg_cmt") | "\n"_cfg));

		c.prim("line") = c.cls(c.ref("comment") | c.ref("exp") | c.ref("raw")) + "\n"_cfg;

		c.prim("file") = c.cls(c.ref("line"));

		c.prep();

		{
			auto src = "Hello world\n";
			EXPECT_EQ(
				parser.parse(src, "file")->compressed()->pre_order_str(),
				"file line raw "
			);
		}

		{
			auto src =
				"This is raw {# variable #} text\n"
				"Hello world {#\n"
				"Multi line\n"
				"#}\n";

			EXPECT_EQ(
				parser .parse(src, "file")->compressed()->pre_order_str(),
				"file "
					"line raw comment beg_cmt end_cmt raw "
					"line raw comment beg_cmt end_cmt "
			);
		}

		{
			auto src =
				"jar saodf {{ 492\n"
				"}}{# fdf #}\n";

			std::ofstream file("gen/ast_node_parse_neg.gv");
			parser.parse(src, "file")->compressed()->debug_dot(file);
		}

	}
}
