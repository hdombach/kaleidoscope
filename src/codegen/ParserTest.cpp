#include "Error.hpp"
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"
#include "SParser.hpp"
#include "AbsoluteSolver.hpp"
#include "util/log.hpp"
#include <fstream>

namespace cg {
	class ParserTest: TestFixture {
		public:
			ParserTest(Test &test, size_t variant): TestFixture(test), _variant(variant) { }

			static size_t variant_count() { return 2; }

			util::Result<void, cg::Error> setup(CfgContext::Ptr &&grammar) {
				if (auto err = grammar->prep().move_or()) {
					return err.value();
				}
				grammar->simplify();
				if (_variant == 0) {
					_parser = SParser::create(std::move(grammar));
				} else if (_variant == 1) {
					if (auto err = AbsoluteSolver::create(std::move(grammar)).move_or(_parser)) {
						return cg::Error(cg::ErrorType::MISC, "Could not setup AbsoluteSolver", err.value());
					}
				} else {
					return cg::Error(cg::ErrorType::MISC, "Invalid variant passed to ParserTest");
				}

				return {};
			}

			CfgContext &cfg() { return _parser->cfg(); }
			Parser &parser() { return *_parser; }

			util::Result<size_t, cg::Error> match(std::string const &str) {
				auto filename = util::f("gen/test-output-", _variant, "-", _count++, ".gv");
				return _parser->match(util::StringRef(str.c_str(), filename.c_str()));
			}

			void parse_eq(
				std::string const &src,
				std::string const &expected
			) {
				auto filename = util::f("gen/test-output-", _variant, "-", _count++, ".gv");
				AstNode *node;
				if (auto err = _parser->parse(util::StringRef(src.c_str(), filename.c_str()), _parser_ctx).move_or(node)) {
					_test.fail("Could not parse provided expression", err.value());
				}

				node->compress(_parser->cfg().prim_names());

				auto file = std::ofstream(filename);
				node->print_dot(file, util::f("Test ", _test.full_name(), " variant ", _variant));

				if (_variant == 1) {
					auto table_file = std::ofstream("gen/test-output-table.txt");
					auto abs = static_cast<abs::AbsoluteSolver*>(_parser.get());
					abs->print_table(table_file);
				}

				_test.expect_eq(node->str_pre_order(), expected);
			}

			void parse_err(
				std::string const &src,
				cg::ErrorType type
			) {
				auto filename = util::f("gen/test-output-", _variant, "-", _count++);
				if (auto res = _parser->parse(util::StringRef(src.c_str(), filename.c_str()), _parser_ctx)) {
					_test.fail(util::f("Expecting parse to fail with ", Error::type_str(type)));
				} else {
					_test.expect_eq(type, res.error().type());
				}
			}

		private:
			Parser::Ptr _parser;
			ParserContext _parser_ctx;
			uint32_t _count = 0;
			uint32_t _variant = 0;
	};

	TEST_F(ParserTest, match_literals) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.root("root") = c["hello"] + T::Eof;
		c.prim("hello") = T::Unmatched;

		EXPECT(f.setup(std::move(ctx)));

		f.parse_eq("UnmatchedToken", "root hello Unmatched EOF ");
	}

	TEST_F(ParserTest, match_number) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.root("root") = c["exp"] + T::Eof;
		c.prim("integer") = T::IntConst | c.empty();
		c.prim("decimal")
			= c["integer"] + T::Period + c["integer"];
		c.prim("exp") = T::ExpB + c["decimal"] + T::ExpE;

		EXPECT(f.setup(std::move(ctx)));

		f.parse_err("{{491f}}", ErrorType::INVALID_PARSE);
		f.parse_err("{{hello}}", ErrorType::INVALID_PARSE);
		f.parse_eq("{{192.}}", "root exp ExpB decimal integer IntConstant Period ExpE EOF ");
		f.parse_eq("{{.89141}}", "root exp ExpB decimal Period integer IntConstant ExpE EOF ");
		f.parse_err("{{..123}}", ErrorType::INVALID_PARSE);
		f.parse_eq("{{.}}", "root exp ExpB decimal Period ExpE EOF ");
		f.parse_eq("{{15.9}}", "root exp ExpB decimal integer IntConstant Period integer IntConstant ExpE EOF ");
	}

	TEST_F(ParserTest, match_math) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.prim("decimal")
			= T::IntConst + T::Period + T::IntConst
			| T::IntConst
			| T::Period + T::IntConst
			| T::IntConst + T::Period;
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

		EXPECT(f.setup(std::move(ctx)));

		f.parse_eq(
			"{{1}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant ExpE EOF "
		);
		f.parse_eq(
			"{{42.1}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant Period IntConstant ExpE EOF "
		);
		f.parse_eq(
			"{{192.12+41}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant Period IntConstant Plus exp_add_sub exp_mult_div exp_inc_dec "
			"exp_sing decimal IntConstant ExpE EOF "
		);
		f.parse_eq(
			"{{192.12+41-0.12}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant Period IntConstant Plus exp_add_sub exp_mult_div exp_inc_dec "
			"exp_sing decimal IntConstant Minus exp_add_sub exp_mult_div exp_inc_dec "
			"exp_sing decimal IntConstant Period IntConstant ExpE EOF "
		);
		f.parse_eq(
			"{{19*1.0}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant Multiply exp_mult_div exp_inc_dec exp_sing decimal IntConstant "
			"Period IntConstant ExpE EOF "
		);
		f.parse_eq(
			"{{19/1.0*20}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant Divide exp_mult_div exp_inc_dec exp_sing decimal IntConstant "
			"Period IntConstant Multiply exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant ExpE EOF "
		);
		f.parse_eq(
			"{{5+2*12}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant Plus exp_add_sub exp_mult_div exp_inc_dec exp_sing "
			"decimal IntConstant Multiply exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant ExpE EOF "
		);
		f.parse_eq(
			"{{5+-2*-+-12}}",
			"root e ExpB exp exp_add_sub exp_mult_div exp_inc_dec exp_sing decimal "
			"IntConstant Plus exp_add_sub exp_mult_div exp_inc_dec Minus exp_inc_dec "
			"exp_sing decimal IntConstant Multiply exp_mult_div exp_inc_dec Minus "
			"exp_inc_dec Plus exp_inc_dec Minus exp_inc_dec exp_sing decimal IntConstant "
			"ExpE EOF "
		);
	}

	TEST_F(ParserTest, simple_exp) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.root("root") = c["S"] + T::Eof; 
		c.prim("S") = T::ExpB + c["E"] + T::ExpE;
		c.prim("E") = c["B"] + T::Mult + c["E"];
		c.prim("E") = c["B"] + T::Plus + c["E"];
		c.prim("E") = c["B"];
		c.prim("B") = T::IntConst;

		EXPECT(f.setup(std::move(ctx)));

		f.parse_eq("{{1*1+2}}", "root S ExpB E B IntConstant Multiply E B IntConstant Plus E B IntConstant ExpE EOF ");
	}

	TEST_F(ParserTest, cls) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.root("value") = c["morse"] + T::Eof;
		c.prim("morse") = T::ExpB + c.cls(T::Period | T::Minus) + T::ExpE;

		EXPECT(f.setup(std::move(ctx)));

		f.parse_err("hello world", ErrorType::INVALID_PARSE);
		f.parse_eq("{{.--.}}", "value morse ExpB Period Minus Minus Period ExpE EOF ");
		f.parse_eq("{{}}", "value morse ExpB ExpE EOF ");
		f.parse_eq("{{.}}", "value morse ExpB Period ExpE EOF ");
	}

	TEST_F(ParserTest, cls_numbers) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.root("root") = c["exp"] + T::Eof;
		c.temp("exp") = T::ExpB + c["num_list"] + T::ExpE;
		c.prim("num_list") = T::IntConst + c.cls(T::Comma + T::IntConst);

		EXPECT(f.setup(std::move(ctx)));

		f.parse_err("hello world", ErrorType::INVALID_PARSE);
		f.parse_err("{{}}", ErrorType::INVALID_PARSE);
		f.parse_eq("{{194}}", "root ExpB num_list IntConstant ExpE EOF ");
		f.parse_eq("{{1,402,215}}", "root ExpB num_list IntConstant Comma IntConstant Comma IntConstant ExpE EOF ");
	}
}
