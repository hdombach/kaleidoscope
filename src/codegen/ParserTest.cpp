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

			util::Result<std::string, cg::Error> parse_str(std::string const &str) {
				auto filename = util::f("gen/test-output-", _variant, "-", _count++, ".gv");
				AstNode *node;
				if (auto err = _parser->parse(util::StringRef(str.c_str(), filename.c_str()), _parser_ctx).move_or(node)) {
					return cg::Error(cg::ErrorType::MISC, "Could not parse string", err.value());
				}

				auto file = std::ofstream(filename);
				node->print_dot(file, util::f("Test ", _test.full_name(), " variant ", _variant));

				if (_variant == 1) {
					auto table_file = std::ofstream("gen/test-output-table.txt");
					auto abs = static_cast<abs::AbsoluteSolver*>(_parser.get());
					abs->print_table(table_file);
				}

				return node->str_pre_order();
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

		auto str = std::string();
		if (auto err = f.parse_str("UnmatchedToken").move_or(str)) {
			FAIL("Could not parse string", err.value());
			return;
		}
		EXPECT_EQ(str, "root hello Unmatched EOF ");
	}
}
