#include <iostream>

namespace cg {
	class TestToken;
	class Token;
}
// Forward declare so plist and test_eq can work
std::ostream &operator<<(std::ostream &os, cg::TestToken const &t);
bool operator==(cg::TestToken const &lhs, cg::Token const &rhs);
bool operator==(cg::Token const &lhs, cg::TestToken const &rhs);
bool operator!=(cg::TestToken const &lhs, cg::Token const &rhs);
bool operator!=(cg::Token const &lhs, cg::TestToken const &rhs);


#include "Tokenizer.hpp"
#include "TemplTokenizer.hpp"
#include "tests/Test.hpp"


namespace cg {
	class TestToken {
		public:
			using Type = TemplTokenType;
			TestToken() = default;
			TestToken(Type type, std::string content):
				_type(type),
				_content(content)
			{}

			static TestToken unmatched(std::string const &s) {
				return TestToken(Type::Unmatched, s);
			}
			static TestToken raw(std::string const &s) {
				return TestToken(Type::Raw, s);
			}
			static TestToken padding(std::string const &s = " ") {
				return TestToken(Type::Pad, s);
			}
			static TestToken newline() {
				return TestToken(Type::Newline, "\n");
			}
			static TestToken comment_b(std::string const &s = "{#") {
				return TestToken(Type::CommentB, s);
			}
			static TestToken comment_e(std::string const &s = "#}") {
				return TestToken(Type::CommentE, s);
			}
			static TestToken exp_b(std::string const &s = "{{") {
				return TestToken(Type::ExpB, s);
			}
			static TestToken exp_e(std::string const &s = "}}") {
				return TestToken(Type::ExpE, s);
			}
			static TestToken stmt_b(std::string const &s = "{\%") {
				return TestToken(Type::StmtB, s);
			}
			static TestToken stmt_e(std::string const &s = "%}") {
				return TestToken(Type::StmtE, s);
			}
			static TestToken if_s() {
				return TestToken(Type::If, "if");
			}
			static TestToken elif_s() {
				return TestToken(Type::Elif, "elif");
			}
			static TestToken else_s() {
				return TestToken(Type::Else, "else");
			}
			static TestToken endif_s() {
				return TestToken(Type::Endif, "endif");
			}
			static TestToken for_s() {
				return TestToken(Type::For, "for");
			}
			static TestToken in_s() {
				return TestToken(Type::In, "in");
			}
			static TestToken endfor_s() {
				return TestToken(Type::EndFor, "endfor");
			}
			static TestToken macro_s() {
				return TestToken(Type::Macro, "macro");
			}
			static TestToken endmacro_s() {
				return TestToken(Type::Endmacro, "endmacro");
			}
			static TestToken include_s() {
				return TestToken(Type::Include, "include");
			}
			static TestToken identifier(std::string const &s) {
				return TestToken(Type::Ident, s);
			}
			static TestToken str_const(std::string const &s) {
				return TestToken(Type::StrConst, "\"" + s + "\"");
			}
			static TestToken int_const(std::string const &s) {
				return TestToken(Type::IntConst, s);
			}
			static TestToken paran_open() {
				return TestToken(Type::ParanOpen, "(");
			}
			static TestToken paran_close() {
				return TestToken(Type::ParanClose, ")");
			}
			static TestToken plus() {
				return TestToken(Type::Plus, "+");
			}
			static TestToken minus() {
				return TestToken(Type::Minus, "-");
			}
			static TestToken exclamation() {
				return TestToken(Type::Excl, "!");
			}
			static TestToken mult() {
				return TestToken(Type::Mult, "*");
			}
			static TestToken div() {
				return TestToken(Type::Div, "/");
			}
			static TestToken perc() {
				return TestToken(Type::Perc, "%");
			}
			static TestToken period() {
				return TestToken(Type::Period, ".");
			}
			static TestToken comma() {
				return TestToken(Type::Comma, ",");
			}
			static TestToken greater_equal() {
				return TestToken(Type::GreatEq, ">=");
			}
			static TestToken greater() {
				return TestToken(Type::Great, ">");
			}
			static TestToken lesser_equal() {
				return TestToken(Type::LessEq, "<=");
			}
			static TestToken lesser() {
				return TestToken(Type::Less, "<");
			}
			static TestToken equal() {
				return TestToken(Type::Equal, "==");
			}
			static TestToken not_equal() {
				return TestToken(Type::NotEqual, "!=");
			}
			static TestToken logical_and() {
				return TestToken(Type::LAnd, "&&");
			}
			static TestToken logical_or() {
				return TestToken(Type::LOr, "||");
			}
			static TestToken bar() {
				return TestToken(Type::Bar, "|");
			}
			static TestToken assignment() {
				return TestToken(Type::Assignment, "=");
			}
			static TestToken eof() {
				return TestToken(Type::Eof, "");
			}

			TemplTokenType type() const { return _type; }
			std::string const &content() const { return _content; }

			std::string str() const {
				return util::f(
					"(",
					TEMPL_TOK_CONFIG.name_table[int(_type)],
					" \"",
					util::escape_str(_content),
					"\")"
				);
			}

			bool matches(Token const &other) const {
				return other.type() == int(type()) && other.content() == content();
			}

		private:
			TemplTokenType _type;
			std::string _content;
	};

	void test_equal(
		Test &test,
		std::vector<Token> const &tokens,
		std::vector<TestToken> const &test_tokens,
		util::FileLocation const &loc = std::source_location::current()
	) {
		auto msgs = std::vector<std::string>();

		auto s = std::min(tokens.size(), test_tokens.size());
		for (auto i = 0; i < s; i++) {
			if (tokens[i] != test_tokens[i]) {
				msgs.push_back(util::f(
					"Mismatch at index ", i, ": ", tokens[i].debug_str(TEMPL_TOK_CONFIG),
					" != ", test_tokens[i].str()
				));
				break;
			}
		}

		if (tokens.size() > s) {
			msgs.push_back("Token list is smaller than expected list");
		} else if (test_tokens.size() > s) {
			msgs.push_back("Expected list is smaller than the token list");
		}

		if (!msgs.empty()) {
			auto msg = std::string("lhs: ");
			auto frag = "[";
			for (auto &token : tokens) {
				msg += frag;
				frag = ", ";
				msg += token.debug_str(TEMPL_TOK_CONFIG);
			}
			msgs.push_back(msg);

			msg = std::string("rhs: ");
			frag = "[";
			for (auto &token : test_tokens) {
				msg += frag;
				frag = ", ";
				msg += token.str();
			}
			msgs.push_back(msg);

			test.fail(msgs, loc);
		} else {
			test.succeed();
		}
	}

	using T = TestToken;

	TEST(tokenizer, raw) {
		auto src =
			"Hello world\n"
			"Wheel chair\n";

		auto res = std::vector{
			TestToken::raw("Hello"),
			TestToken::padding(),
			TestToken::raw("world"),
			TestToken::newline(),
			TestToken::raw("Wheel"),
			TestToken::padding(),
			TestToken::raw("chair"),
			TestToken::newline(),
			TestToken::eof(),
		};
		auto tokens = tokenize_templ(src);
		test_equal(_test, tokens, res);
	}

	TEST(tokenizer, comment) {
		auto src =
			"ahhh\n"
			"first {# temp. #} time.\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("ahhh"),
			TestToken::newline(),
			TestToken::raw("first"),
			TestToken::padding(),
			TestToken::comment_b(),
			TestToken::padding(),
			TestToken::raw("temp."),
			TestToken::padding(),
			TestToken::comment_e(),
			TestToken::padding(),
			TestToken::raw("time."),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);

	}

	TEST(tokenizer, str_constants) {
		auto src =
			"His name is {{ \"John Cena!\" }}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("His"),
			TestToken::padding(),
			TestToken::raw("name"),
			TestToken::padding(),
			TestToken::raw("is"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::padding(),
			TestToken::str_const("John Cena!"),
			TestToken::padding(),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, int_constants) {
		auto src =
			"The age is {{ 69 }}.\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("The"),
			TestToken::padding(),
			TestToken::raw("age"),
			TestToken::padding(),
			TestToken::raw("is"),
			TestToken::padding(),
			TestToken::exp_b("{{"),
			TestToken::padding(),
			TestToken::int_const("69"),
			TestToken::padding(),
			TestToken::exp_e(),
			TestToken::raw("."),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);

		src =
			"{{023}}";
		tokens = tokenize_templ(src);
		expected = std::vector{
			TestToken::exp_b(),
			TestToken::int_const("023"),
			TestToken::exp_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, paranthesis) {
		auto src =
			"{{((get_method()))()}}";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::exp_b(),
			TestToken::paran_open(),
			TestToken::paran_open(),
			TestToken::identifier("get_method"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::paran_close(),
			TestToken::paran_close(),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::exp_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, addition) {
		auto src =
			"The sum is {{5 + weird}}\n\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("The"),
			TestToken::padding(),
			TestToken::raw("sum"),
			TestToken::padding(),
			TestToken::raw("is"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::int_const("5"),
			TestToken::padding(),
			TestToken::plus(),
			TestToken::padding(),
			TestToken::identifier("weird"),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, subtraction) {
		auto src =
			"The sum is {{9-1  -  \t  2}}";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("The"),
			TestToken::padding(),
			TestToken::raw("sum"),
			TestToken::padding(),
			TestToken::raw("is"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::int_const("9"),
			TestToken::minus(),
			TestToken::int_const("1"),
			TestToken::padding("  "),
			TestToken::minus(),
			TestToken::padding("  \t  "),
			TestToken::int_const("2"),
			TestToken::exp_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}


	TEST(tokenizer, multiplication) {
		auto src =
			"The product is {{501*prime}}";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("The"),
			TestToken::padding(),
			TestToken::raw("product"),
			TestToken::padding(),
			TestToken::raw("is"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::int_const("501"),
			TestToken::mult(),
			TestToken::identifier("prime"),
			TestToken::exp_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, division) {
		auto src =
			"Rounding down: \n\t{{ value / 10 }}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Rounding"),
			TestToken::padding(),
			TestToken::raw("down:"),
			TestToken::padding(),
			TestToken::newline(),
			TestToken::padding("\t"),
			TestToken::exp_b(),
			TestToken::padding(),
			TestToken::identifier("value"),
			TestToken::padding(),
			TestToken::div(),
			TestToken::padding(),
			TestToken::int_const("10"),
			TestToken::padding(),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, modulus) {
		auto src =
			"The mod is: {{sum%10}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("The"),
			TestToken::padding(),
			TestToken::raw("mod"),
			TestToken::padding(),
			TestToken::raw("is:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("sum"),
			TestToken::perc(),
			TestToken::int_const("10"),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, member) {
		auto src =
			"cur_month={{date.month.raw()}}.\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("cur_month="),
			TestToken::exp_b(),
			TestToken::identifier("date"),
			TestToken::period(),
			TestToken::identifier("month"),
			TestToken::period(),
			TestToken::identifier("raw"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::exp_e(),
			TestToken::raw("."),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, args) {
		auto src =
			"# Summary:\n"
			"- Sum: {{sum(value1, value2,value3}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("#"),
			TestToken::padding(),
			TestToken::raw("Summary:"),
			TestToken::newline(),
			TestToken::raw("-"),
			TestToken::padding(),
			TestToken::raw("Sum:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("sum"),
			TestToken::paran_open(),
			TestToken::identifier("value1"),
			TestToken::comma(),
			TestToken::padding(),
			TestToken::identifier("value2"),
			TestToken::comma(),
			TestToken::identifier("value3"),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, greater_equal) {
		auto src =
			"Can drink: {{age>=21}}!\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Can"),
			TestToken::padding(),
			TestToken::raw("drink:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::greater_equal(),
			TestToken::int_const("21"),
			TestToken::exp_e(),
			TestToken::raw("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, greater) {
		auto src =
			"Can drink: {{age>20}}!\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Can"),
			TestToken::padding(),
			TestToken::raw("drink:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::greater(),
			TestToken::int_const("20"),
			TestToken::exp_e(),
			TestToken::raw("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, lesser_equal) {
		auto src =
			"Underage: {{age<=20}}!\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Underage:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::lesser_equal(),
			TestToken::int_const("20"),
			TestToken::exp_e(),
			TestToken::raw("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, lesser) {
		auto src =
			"Underage: {{age<21}}!\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Underage:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::lesser(),
			TestToken::int_const("21"),
			TestToken::exp_e(),
			TestToken::raw("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, equal) {
		auto src =
			"Todays birthday?: {{age==date()}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Todays"),
			TestToken::padding(),
			TestToken::raw("birthday?:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::equal(),
			TestToken::identifier("date"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, not_equal) {
		auto src =
			"Todays non-birthday?: {{age!=date()}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Todays"),
			TestToken::padding(),
			TestToken::raw("non-birthday?:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::not_equal(),
			TestToken::identifier("date"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, logical_not) {
		auto src =
			"normal user: {{!!!is_admin}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("normal"),
			TestToken::padding(),
			TestToken::raw("user:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::exclamation(),
			TestToken::exclamation(),
			TestToken::exclamation(),
			TestToken::identifier("is_admin"),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, logical_and) {
		auto src =
			"test = {{test1 && (test2&&test3)}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("test"),
			TestToken::padding(),
			TestToken::raw("="),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("test1"),
			TestToken::padding(),
			TestToken::logical_and(),
			TestToken::padding(),
			TestToken::paran_open(),
			TestToken::identifier("test2"),
			TestToken::logical_and(),
			TestToken::identifier("test3"),
			TestToken::paran_close(),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, logical_or) {
		auto src =
			"test = {{(test1)|| test2||test3}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("test"),
			TestToken::padding(),
			TestToken::raw("="),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::paran_open(),
			TestToken::identifier("test1"),
			TestToken::paran_close(),
			TestToken::logical_or(),
			TestToken::padding(),
			TestToken::identifier("test2"),
			TestToken::logical_or(),
			TestToken::identifier("test3"),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, filter) {
		auto src =
			"==quote of the day==\n"
			"{{get_quote()|trim|indent(\"> \")}}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("==quote"),
			TestToken::padding(),
			TestToken::raw("of"),
			TestToken::padding(),
			TestToken::raw("the"),
			TestToken::padding(),
			TestToken::raw("day=="),
			TestToken::newline(),
			TestToken::exp_b(),
			TestToken::identifier("get_quote"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::bar(),
			TestToken::identifier("trim"),
			TestToken::bar(),
			TestToken::identifier("indent"),
			TestToken::paran_open(),
			TestToken::str_const("> "),
			TestToken::paran_close(),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, if_statement) {
		auto src =
			"Welcome\n"
			"{\%if is_admin() %}\n"
			"<button action=\"delete_everything\">\n"
			"{\% endif %}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Welcome"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::if_s(),
			TestToken::padding(),
			TestToken::identifier("is_admin"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::raw("<button"),
			TestToken::padding(),
			TestToken::raw("action=\"delete_everything\">"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::endif_s(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, if_else_chain) {
		auto src = ""
			"{\% if value==3%}\n"
			"- value=3\n"
			"{\%elif value==4%}\n"
			"- value=4\n"
			"{\% else %}\n"
			"- idk\n"
			"{\% endif %}\n"
			"";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::if_s(),
			TestToken::padding(),
			TestToken::identifier("value"),
			TestToken::equal(),
			TestToken::int_const("3"),
			TestToken::stmt_e(),
			TestToken::raw("-"),
			TestToken::padding(),
			TestToken::raw("value=3"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::elif_s(),
			TestToken::padding(),
			TestToken::identifier("value"),
			TestToken::equal(),
			TestToken::int_const("4"),
			TestToken::stmt_e(),
			TestToken::raw("-"),
			TestToken::padding(),
			TestToken::raw("value=4"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::else_s(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::raw("-"),
			TestToken::padding(),
			TestToken::raw("idk"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::endif_s(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, for_loop) {
		auto src =
			"Snooping list:\n"
			"{\% for item in list|filter%}\n"
			"- {{item}}.\n"
			"{\%endfor%}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::raw("Snooping"),
			TestToken::padding(),
			TestToken::raw("list:"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::for_s(),
			TestToken::padding(),
			TestToken::identifier("item"),
			TestToken::padding(),
			TestToken::in_s(),
			TestToken::padding(),
			TestToken::identifier("list"),
			TestToken::bar(),
			TestToken::identifier("filter"),
			TestToken::stmt_e(),
			TestToken::raw("-"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("item"),
			TestToken::exp_e(),
			TestToken::raw("."),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::endfor_s(),
			TestToken::stmt_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, macro) {
		auto src =
			"{\% macro(say_hello,prefix=\"----\") %}\n"
			"{{prefix}}{{say_hello}}\n"
			"{\% endmacro %}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::macro_s(),
			TestToken::paran_open(),
			TestToken::identifier("say_hello"),
			TestToken::comma(),
			TestToken::identifier("prefix"),
			TestToken::assignment(),
			TestToken::str_const("----"),
			TestToken::paran_close(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::exp_b(),
			TestToken::identifier("prefix"),
			TestToken::exp_e(),
			TestToken::exp_b(),
			TestToken::identifier("say_hello"),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::endmacro_s(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, include) {
		auto src =
			"{\%include\"test.cpp\"%}\n";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::stmt_b(),
			TestToken::include_s(),
			TestToken::str_const("test.cpp"),
			TestToken::stmt_e(),
			TestToken::eof(),
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, default_padding) {
		auto src =
			"{\% if true %}\n"
			"\n"
			"Hello world\n"
			"{\% endif %}\n"
			"";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::if_s(),
			TestToken::padding(),
			TestToken::identifier("true"),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::newline(),
			TestToken::raw("Hello"),
			TestToken::padding(),
			TestToken::raw("world"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::endif_s(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::eof()
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, leading_padding) {
		auto src =
			"\n"
			"{\% if foo %}\n"
			"-\n"
			"{\% endif %}"
			"";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::newline(),
			T::stmt_b(),
			T::padding(),
			T::if_s(),
			T::padding(),
			T::identifier("foo"),
			T::padding(),
			T::stmt_e(),
			T::raw("-"),
			T::newline(),
			T::stmt_b(),
			T::padding(),
			T::endif_s(),
			T::padding(),
			T::stmt_e(),
			T::eof()
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, extra_padding) {
		auto src =
			"\n"
			"\t {\% if foo %}  \n"
			"\t oy\n"
			"   {\% endif %}\t\n"
			"";
		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::newline(),
			T::stmt_b(),
			T::padding(),
			T::if_s(),
			T::padding(),
			T::identifier("foo"),
			T::padding(),
			T::stmt_e(),
			T::padding("\t "),
			T::raw("oy"),
			T::newline(),
			T::stmt_b(),
			T::padding(),
			T::endif_s(),
			T::padding(),
			T::stmt_e(),
			T::eof()
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, remove_padding) {
		auto src =
			"\n\n"
			"\t {\%- if foo -%}   \n"
			"    hello\n"
			"    world\n"
			"{\%- endif -%}\n"
			"";

		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::stmt_b("{\%-"),
			T::padding(),
			T::if_s(),
			T::padding(),
			T::identifier("foo"),
			T::padding(),
			T::stmt_e("-%}"),
			T::raw("hello"),
			T::newline(),
			T::padding("    "),
			T::raw("world"),
			T::stmt_b("{\%-"),
			T::padding(),
			T::endif_s(),
			T::padding(),
			T::stmt_e("-%}"),
			T::eof()
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, keep_padding) {
		auto src =
			"\n\n"
			"\t {\%+ if foo +%}   \n"
			"    hello\n"
			"    world\n"
			"{\%+ endif +%}\n"
			"";

		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::newline(),
			T::newline(),
			T::padding("\t "),
			T::stmt_b("{\%+"),
			T::padding(),
			T::if_s(),
			T::padding(),
			T::identifier("foo"),
			T::padding(),
			T::stmt_e("+%}"),
			T::padding("   "),
			T::newline(),
			T::padding("    "),
			T::raw("hello"),
			T::newline(),
			T::padding("    "),
			T::raw("world"),
			T::newline(),
			T::stmt_b("{\%+"),
			T::padding(),
			T::endif_s(),
			T::padding(),
			T::stmt_e("+%}"),
			T::newline(),
			T::eof()
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, exp_default_padding) {
		auto src =
			"\n\n"
			"  {{5}}\n"
			"";

		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::newline(),
			T::newline(),
			T::padding("  "),
			T::exp_b(),
			T::int_const("5"),
			T::exp_e(),
			T::newline(),
			T::eof()
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, exp_keep_padding) {
		auto src =
			"\n\n"
			"  {{+5+}}\n"
			"  \n"
			"{{ +5 }}\n"
			"";

		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::newline(),
			T::newline(),
			T::padding("  "),
			T::exp_b("{{+"),
			T::int_const("5"),
			T::exp_e("+}}"),
			T::newline(),
			T::padding("  "),
			T::newline(),
			T::exp_b(),
			T::padding(),
			T::plus(),
			T::int_const("5"),
			T::padding(),
			T::exp_e(),
			T::newline(),
			T::eof()
		};
		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, exp_remove_padding) {
		auto src =
			"\n\n"
			"  {{-5-}}\n"
			"  \n"
			"{{ -5 }}\n"
			"";

		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::exp_b("{{-"),
			T::int_const("5"),
			T::exp_e("-}}"),
			T::exp_b(),
			T::padding(),
			T::minus(),
			T::int_const("5"),
			T::padding(),
			T::exp_e(),
			T::newline(),
			T::eof()
		};

		test_equal(_test, tokens, expected);
	}

	TEST(tokenizer, indented_loop) {
		auto src =
			"namespace {\n"
			"\t{\% for name in names %}\n"
			"uint8_t {{name}};\n"
			"\t{\% endfor %}\n"
			"}\n"
			"";

		auto tokens = tokenize_templ(src);
		auto expected = std::vector{
			T::raw("namespace"),
			T::padding(),
			T::raw("{"),
			T::newline(),
			T::stmt_b(),
			T::padding(),
			T::for_s(),
			T::padding(),
			T::identifier("name"),
			T::padding(),
			T::in_s(),
			T::padding(),
			T::identifier("names"),
			T::padding(),
			T::stmt_e(),
			T::raw("uint8_t"),
			T::padding(),
			T::exp_b(),
			T::identifier("name"),
			T::exp_e(),
			T::raw(";"),
			T::newline(),
			T::stmt_b(),
			T::padding(),
			T::endfor_s(),
			T::padding(),
			T::stmt_e(),
			T::raw("}"),
			T::newline(),
			T::eof(),
		};

		test_equal(_test, tokens, expected);
	}
}

inline std::ostream &operator<<(std::ostream &os, cg::TestToken const &t) {
	return os << t.str();
}

inline bool operator==(cg::Token const &lhs, cg::TestToken const &rhs) {
	return rhs.matches(lhs);
}

inline bool operator==(cg::TestToken const &lhs, cg::Token const &rhs) {
	return lhs.matches(rhs);
}

inline bool operator!=(cg::Token const &lhs, cg::TestToken const &rhs) {
	return !rhs.matches(lhs);
}

inline bool operator!=(cg::TestToken const &lhs, cg::Token const &rhs) {
	return !lhs.matches(rhs);
}
