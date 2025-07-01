
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"
#include "SParser.hpp"
#include "util/log.hpp"
#include "AstNode.hpp"
#include "util/StringRef.hpp"

#include <fstream>

namespace cg {
	class AstNodeTest: TestFixture {
		public:
			AstNodeTest(Test &test, size_t variant): TestFixture(test) {
				_parser = SParser::create(CfgContext::create());
				_should_simplify = variant;
			}
			//Make sure you don't copy this because parser has pointer to cfg.
			AstNodeTest(AstNodeTest const &other) = delete;

			std::string suite_name() { return "UNKNOWN"; }
			static size_t variant_count() { return 2; }

			CfgContext &cfg() { return _parser->cfg(); }
			Parser &parser() { return *_parser; }

			auto match(std::string const &str) {
				if (!_is_prepped) {
					_parser->cfg().prep();
					if (_should_simplify) {
						_parser->cfg().simplify();
					}

					_is_prepped = true;
				}
				auto file = "temp-file-" + std::to_string(count++);
				return _parser->match(util::StringRef(str.c_str(), file.c_str()));
			}

		private:
			bool _should_simplify;
			Parser::Ptr _parser;
			bool _is_prepped = false;
			uint32_t count;
	};

	TEST_F(AstNodeTest, match_literals) {
		auto &c = f.cfg();
		using T = Token::Type;

		log_debug() << "size: " << c.cfg_rule_sets().size() << std::endl;
		c.root("root") = c["hello"] + T::Eof;
		c.prim("hello") = T::Unmatched;
		log_debug() << "size: " << c.cfg_rule_sets().size() << std::endl;

		c.prep().value();

		EXPECT_EQ(f.match("Hello").value(), 5);
		EXPECT_KERROR(f.match("{\% %}"), KError::Type::CODEGEN);
	}

	TEST_F(AstNodeTest, match_number) {
		auto &c = f.cfg();
		using T = Token::Type;

		c.root("root") = c["exp"] + T::Eof;
		c.prim("integer") = T::IntConst | c.empty();
		c.prim("decimal")
			= c["integer"] + T::Period + c["integer"];
		c.prim("exp") = T::ExpB + c["decimal"] + T::ExpE;

		EXPECT(c.prep());

		EXPECT_KERROR(f.match("{{491f}}"), KError::Type::CODEGEN);
		EXPECT_KERROR(f.match("{{hello}}"), KError::Type::CODEGEN);
		EXPECT_EQ(f.match("{{192.}}").value(), 8);
		EXPECT_EQ(f.match("{{.89141}}").value(), 10);
		EXPECT_KERROR(f.match("{{..123}}"), KError::Type::CODEGEN);
		EXPECT_EQ(f.match("{{.}}").value(), 5);
		EXPECT_EQ(f.match("{{15.9}}").value(), 8);
	}

	TEST_F(AstNodeTest, match_math) {
		auto &c = f.cfg();
		using T = Token::Type;

		c.prim("integer") = T::IntConst | c.empty();
		c.prim("decimal")
			= c["integer"] + T::Period + c["integer"]
			| c["integer"];
		c.prim("exp_sing") = c["decimal"];
		c.prim("exp_inc_dec")
			= T::Plus + c["exp_inc_dec"]
			| T::Minus + c["exp_inc_dec"]
			| T::Excl + c["exp_inc_dec"]
			| c["exp_sing"];
		c.prim("exp_mult_div")
			= c["exp_inc_dec"] + T::Mult + c["exp_mult_div"]
			| c["exp_inc_dec"] + T::Div + c["exp_mult_div"]
			| c["exp_inc_dec"] + T::Perc + c["exp_mult_div"]
			| c["exp_inc_dec"];
		c.prim("exp_add_sub")
			= c["exp_mult_div"] + T::Plus + c["exp_add_sub"]
			| c["exp_mult_div"] + T::Minus + c["exp_add_sub"]
			| c["exp_mult_div"];
		c.prim("exp") = c["exp_add_sub"];

		c.prim("e") = T::ExpB + c["exp"] + T::ExpE;
		c.root("root") = c["e"] + T::Eof;

		c.prep().value();

		EXPECT_EQ(f.match("{{1}}").value(), 5);
		EXPECT_EQ(f.match("{{42.1}}").value(), 8);
		EXPECT_EQ(f.match("{{192.12+41}}").value(), 13);
		EXPECT_EQ(f.match("{{192.12+41-0.12}}").value(), 18);
		EXPECT_EQ(f.match("{{19*1.0}}").value(), 10);
		EXPECT_EQ(f.match("{{19/1.0*20}}").value(), 13);
		EXPECT_EQ(f.match("{{5+2*12}}").value(), 10);
		EXPECT_EQ(f.match("{{5+-2*-+-12}}").value(), 14);
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
