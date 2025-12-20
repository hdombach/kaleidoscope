#include "Error.hpp"
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

		c.root("root") = c["hello"] + T::Eof;
		c.prim("hello") = T::Unmatched;

		c.prep().value();

		EXPECT_EQ(f.match("Hello").value(), 5);
		EXPECT_TERROR(f.match("{\% %}"), cg::ErrorType::INVALID_PARSE);
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

		EXPECT_TERROR(f.match("{{491f}}"), ErrorType::INVALID_PARSE);
		EXPECT_TERROR(f.match("{{hello}}"), ErrorType::INVALID_PARSE);
		EXPECT_EQ(f.match("{{192.}}").value(), 8);
		EXPECT_EQ(f.match("{{.89141}}").value(), 10);
		EXPECT_TERROR(f.match("{{..123}}"), ErrorType::INVALID_PARSE);
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
}
