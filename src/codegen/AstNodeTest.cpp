
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"
#include "SParser.hpp"
#include "util/log.hpp"
#include "AstNode.hpp"

#include <fstream>

namespace cg {
	class AstNodeTest: TestFixture {
		public:
			AstNodeTest(Test &test, size_t variant): TestFixture(test), _cfg(), _parser(_cfg) {
				_should_simplify = variant;
			}
			//Make sure you don't copy this because parser has pointer to cfg.
			AstNodeTest(AstNodeTest const &other) = delete;

			std::string suite_name() { return "UNKNOWN"; }
			static size_t variant_count() { return 2; }

			CfgContext &cfg() { return _cfg; }
			SParser &parser() { return _parser; }

			auto match(std::string const &str, std::string const &root) {
				if (!_is_prepped) {
					_cfg.prep();
					if (_should_simplify) {
						_cfg.simplify();
					}

					_is_prepped = true;
				}
				return _parser.match(str, root);
			}

		private:
			bool _should_simplify;
			CfgContext _cfg;
			SParser _parser;
			bool _is_prepped = false;
	};

	TEST_F(AstNodeTest, match_literals) {
		auto &c = f.cfg();

		log_debug() << "size: " << c.cfg_rule_sets().size() << std::endl;
		c.root("hello") = c.s("Hello") + c.eof();
		log_debug() << "size: " << c.cfg_rule_sets().size() << std::endl;

		EXPECT(c.prep());

		EXPECT_EQ(f.match("Hello", "hello").value(), 5);
		EXPECT_KERROR(f.match("hello", "hello"), KError::Type::CODEGEN);
		EXPECT_KERROR(f.match("Helloo", "hello"), KError::Type::CODEGEN);
	}

	TEST_F(AstNodeTest, match_number) {
		auto &c = f.cfg();

		c.prim("digit") = c.i("0123456789");
		c.prim("integer")
			= c["digit"] + c["integer"]
			| c.s("");
		c.root("decimal") = c["integer"] + c.s(".") + c["integer"];

		EXPECT(c.prep());

		EXPECT_KERROR(f.match("491f", "decimal"), KError::Type::CODEGEN);
		EXPECT_KERROR(f.match("hello", "decimal"), KError::Type::CODEGEN);
		EXPECT_EQ(f.match("192.", "decimal").value(), 4);
		EXPECT_EQ(f.match(".89141", "decimal").value(), 6);
		EXPECT_KERROR(f.match("..123", "decimal"), KError::Type::CODEGEN);
		EXPECT_EQ(f.match(".", "decimal").value(), 1);
		EXPECT_EQ(f.match("15.9", "decimal").value(), 4);
	}

	TEST_F(AstNodeTest, match_math) {
		auto &c = f.cfg();

		c.prim("digit") =
			c.s("0") | c.s("1") | c.s("2") | c.s("3") | c.s("4") |
			c.s("5") | c.s("6") | c.s("7") | c.s("8") | c.s("9");
		c.prim("integer")
			= c["digit"] + c["integer"]
			| c.s("");
		c.prim("decimal")
			= c["integer"] + c.s(".") + c["integer"]
			| c["integer"];
		c.prim("exp_sing") = c["decimal"];
		c.prim("exp_inc_dec")
			= c.s("+") + c["exp_inc_dec"]
			| c.s("-") + c["exp_inc_dec"]
			| c.s("!") + c["exp_inc_dec"]
			| c["exp_sing"];
		c.prim("exp_mult_div")
			= c["exp_inc_dec"] + c.s("*") + c["exp_mult_div"]
			| c["exp_inc_dec"] + c.s("/") + c["exp_mult_div"]
			| c["exp_inc_dec"] + c.s("%") + c["exp_mult_div"]
			| c["exp_inc_dec"];
		c.prim("exp_add_sub")
			= c["exp_mult_div"] + c.s("+") + c["exp_add_sub"]
			| c["exp_mult_div"] + c.s("-") + c["exp_add_sub"]
			| c["exp_mult_div"];
		c.root("exp") = c["exp_add_sub"];

		EXPECT(c.prep());

		EXPECT_EQ(f.match("1", "exp").value(), 1);
		EXPECT_EQ(f.match("42.1", "exp").value(), 4);
		EXPECT_EQ(f.match("192.12+41", "exp").value(), 9);
		EXPECT_EQ(f.match("192.12+41-0.12", "exp").value(), 14);
		EXPECT_EQ(f.match("19*1.0", "exp").value(), 6);
		EXPECT_EQ(f.match("19/1.0*20", "exp").value(), 9);
		EXPECT_EQ(f.match("5+2*12", "exp").value(), 6);
		EXPECT_EQ(f.match("5+-2*-+-12", "exp").value(), 10);
	}

/*
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
			parser.parse("1", "exp")->compressed()->trimmed().pre_order_str(),
			"exp add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		EXPECT_EQ(
			parser.parse("501.76", "exp")->compressed()->trimmed().pre_order_str(),
			"exp add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		EXPECT_EQ(
			parser.parse("41.2+14", "exp")->compressed()->trimmed().pre_order_str(),
			"exp add_sub_exp "
			"mult_div_exp unary_exp sing_exp "
			"add_sub_exp mult_div_exp unary_exp sing_exp "
		);

		std::ofstream file("gen/ast_node_parse_math.gv");
		parser.parse("4-3*82  /3+ 2.3", "exp")->compressed()->trimmed().debug_dot(file);
		file.close();

		EXPECT_EQ(
			parser.parse("4-3*82  /3+ 2.3", "exp")->compressed()->trimmed().pre_order_str(),
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
			parser.parse("5+-2*-+-12", "exp")->compressed()->trimmed().pre_order_str(),
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
				parser.parse(src, "file")->compressed()->trimmed().pre_order_str(),
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
				parser .parse(src, "file")->compressed()->trimmed().pre_order_str(),
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
	*/
}
