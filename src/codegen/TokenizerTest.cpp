#include "tests/Test.hpp"
#include "Tokenizer.hpp"
#include "util/PrintTools.hpp"
#include "util/log.hpp"


namespace cg {
	class TestToken {
		public:
			using Type = Token::Type;
			TestToken() = default;
			TestToken(Type type, std::string content):
				_type(type),
				_content(content)
			{}

			static TestToken unmatched(std::string const &s) {
				return TestToken(Type::Unmatched, s);
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

			Token::Type type() const { return _type; }
			std::string const &content() const { return _content; }

			std::string str() const {
				return util::f("(", Token::type_str(_type), " \"", util::escape_str(_content), "\")");
			}

		private:
			Token::Type _type;
			std::string _content;
	};

#define EXPECT_TOKEN(lhs, rhs) {\
	EXPECT_EQ(lhs.size(), rhs.size()); \
	if (lhs.size() == rhs.size()) { \
		for (int i = 0; i < lhs.size(); i++) { \
			EXPECT_EQ(lhs[i].type(), rhs[i].type()); \
			EXPECT_EQ(lhs[i].content(), rhs[i].content()); \
		} \
	}  \
}


	TEST(tokenizer, raw) {
		auto src =
			"Hello world\n"
			"Wheel chair\n";

		auto res = std::vector{
			TestToken::unmatched("Hello"),
			TestToken::padding(),
			TestToken::unmatched("world"),
			TestToken::newline(),
			TestToken::unmatched("Wheel"),
			TestToken::padding(),
			TestToken::unmatched("chair"),
			TestToken::newline(),
			TestToken::eof(),
		};
		auto tokens = simplify_tokens(tokenize(src));
		EXPECT_TOKEN(tokens, res);
	}

	TEST(tokenizer, comment) {
		auto src =
			"ahhh\n"
			"first {# temp #} time.\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("ahhh"),
			TestToken::newline(),
			TestToken::unmatched("first"),
			TestToken::padding(),
			TestToken::comment_b(),
			TestToken::padding(),
			TestToken::identifier("temp"),
			TestToken::padding(),
			TestToken::comment_e(),
			TestToken::padding(),
			TestToken::unmatched("time."),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);

	}

	TEST(tokenizer, str_constants) {
		auto src =
			"His name is {{ \"John Cena!\" }}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("His"),
			TestToken::padding(),
			TestToken::unmatched("name"),
			TestToken::padding(),
			TestToken::unmatched("is"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::padding(),
			TestToken::str_const("John Cena!"),
			TestToken::padding(),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, int_constants) {
		auto src =
			"The age is {{ 69 }}.\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("The"),
			TestToken::padding(),
			TestToken::unmatched("age"),
			TestToken::padding(),
			TestToken::unmatched("is"),
			TestToken::padding(),
			TestToken::exp_b("{{"),
			TestToken::padding(),
			TestToken::int_const("69"),
			TestToken::padding(),
			TestToken::exp_e(),
			TestToken::unmatched("."),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);

		src =
			"{{023}}";
		tokens = simplify_tokens(tokenize(src));
		expected = std::vector{
			TestToken::exp_b(),
			TestToken::int_const("023"),
			TestToken::exp_e(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, paranthesis) {
		auto src =
			"{{((get_method()))()}}";
		auto tokens = simplify_tokens(tokenize(src));
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, addition) {
		auto src =
			"The sum is {{5 + weird}}\n\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("The"),
			TestToken::padding(),
			TestToken::unmatched("sum"),
			TestToken::padding(),
			TestToken::unmatched("is"),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, subtraction) {
		auto src =
			"The sum is {{9-1  -  \t  2}}";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("The"),
			TestToken::padding(),
			TestToken::unmatched("sum"),
			TestToken::padding(),
			TestToken::unmatched("is"),
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
		EXPECT_TOKEN(tokens, expected);
	}


	TEST(tokenizer, multiplication) {
		auto src =
			"The product is {{501*prime}}";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("The"),
			TestToken::padding(),
			TestToken::unmatched("product"),
			TestToken::padding(),
			TestToken::unmatched("is"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::int_const("501"),
			TestToken::mult(),
			TestToken::identifier("prime"),
			TestToken::exp_e(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, division) {
		auto src =
			"Rounding down: \n\t{{ value / 10 }}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Rounding"),
			TestToken::padding(),
			TestToken::unmatched("down:"),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, modulus) {
		auto src =
			"The mod is: {{sum%10}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("The"),
			TestToken::padding(),
			TestToken::unmatched("mod"),
			TestToken::padding(),
			TestToken::unmatched("is:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("sum"),
			TestToken::perc(),
			TestToken::int_const("10"),
			TestToken::exp_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, member) {
		auto src =
			"cur_month={{date.month.raw()}}.\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("cur_month="),
			TestToken::exp_b(),
			TestToken::identifier("date"),
			TestToken::period(),
			TestToken::identifier("month"),
			TestToken::period(),
			TestToken::identifier("raw"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::exp_e(),
			TestToken::unmatched("."),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, args) {
		auto src =
			"# Summary:\n"
			"- Sum: {{sum(value1, value2,value3}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("#"),
			TestToken::padding(),
			TestToken::unmatched("Summary:"),
			TestToken::newline(),
			TestToken::unmatched("-"),
			TestToken::padding(),
			TestToken::unmatched("Sum:"),
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
	}

	TEST(tokenizer, greater_equal) {
		auto src =
			"Can drink: {{age>=21}}!\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Can"),
			TestToken::padding(),
			TestToken::unmatched("drink:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::greater_equal(),
			TestToken::int_const("21"),
			TestToken::exp_e(),
			TestToken::unmatched("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, greater) {
		auto src =
			"Can drink: {{age>20}}!\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Can"),
			TestToken::padding(),
			TestToken::unmatched("drink:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::greater(),
			TestToken::int_const("20"),
			TestToken::exp_e(),
			TestToken::unmatched("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, lesser_equal) {
		auto src =
			"Underage: {{age<=20}}!\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Underage:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::lesser_equal(),
			TestToken::int_const("20"),
			TestToken::exp_e(),
			TestToken::unmatched("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, lesser) {
		auto src =
			"Underage: {{age<21}}!\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Underage:"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("age"),
			TestToken::lesser(),
			TestToken::int_const("21"),
			TestToken::exp_e(),
			TestToken::unmatched("!"),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, equal) {
		auto src =
			"Todays birthday?: {{age==date()}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Todays"),
			TestToken::padding(),
			TestToken::unmatched("birthday?:"),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, not_equal) {
		auto src =
			"Todays non-birthday?: {{age!=date()}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Todays"),
			TestToken::padding(),
			TestToken::unmatched("non-birthday?:"),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, logical_not) {
		auto src =
			"normal user: {{!!!is_admin}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("normal"),
			TestToken::padding(),
			TestToken::unmatched("user:"),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, logical_and) {
		auto src =
			"test = {{test1 && (test2&&test3)}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("test"),
			TestToken::padding(),
			TestToken::unmatched("="),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, logical_or) {
		auto src =
			"test = {{(test1)|| test2||test3}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("test"),
			TestToken::padding(),
			TestToken::unmatched("="),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, filter) {
		auto src =
			"==quote of the day==\n"
			"{{get_quote()|trim|indent(\"> \")}}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("==quote"),
			TestToken::padding(),
			TestToken::unmatched("of"),
			TestToken::padding(),
			TestToken::unmatched("the"),
			TestToken::padding(),
			TestToken::unmatched("day=="),
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
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, if_statement) {
		auto src =
			"Welcome\n"
			"{\%if is_admin() %}\n"
			"<button action=\"delete_everything\">"
			"{\%- endif +%}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Welcome"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::if_s(),
			TestToken::padding(),
			TestToken::identifier("is_admin"),
			TestToken::paran_open(),
			TestToken::paran_close(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::newline(),
			TestToken::unmatched("<button"),
			TestToken::padding(),
			TestToken::unmatched("action=\"delete_everything\">"),
			TestToken::stmt_b("{\%-"),
			TestToken::padding(),
			TestToken::endif_s(),
			TestToken::padding(),
			TestToken::stmt_e("+%}"),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, if_else_chain) {
		auto src = ""
			"{\% if value==3%}\n"
			"- value=3\n"
			"{\%+elif value==4+%}\n"
			"- value=4\n"
			"{\% else %}"
			"- idk\n"
			"{\% endif %}\n"
			"";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::if_s(),
			TestToken::padding(),
			TestToken::identifier("value"),
			TestToken::equal(),
			TestToken::int_const("3"),
			TestToken::stmt_e(),
			TestToken::newline(),
			TestToken::unmatched("-"),
			TestToken::padding(),
			TestToken::unmatched("value=3"),
			TestToken::newline(),
			TestToken::stmt_b("{\%+"),
			TestToken::elif_s(),
			TestToken::padding(),
			TestToken::identifier("value"),
			TestToken::equal(),
			TestToken::int_const("4"),
			TestToken::stmt_e("+%}"),
			TestToken::newline(),
			TestToken::unmatched("-"),
			TestToken::padding(),
			TestToken::unmatched("value=4"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::else_s(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::unmatched("-"),
			TestToken::padding(),
			TestToken::unmatched("idk"),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::padding(),
			TestToken::endif_s(),
			TestToken::padding(),
			TestToken::stmt_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, for_loop) {
		auto src =
			"Snooping list:\n"
			"{\% for item in list|filter%}\n"
			"- {{item}}.\n"
			"{\%endfor%}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::unmatched("Snooping"),
			TestToken::padding(),
			TestToken::unmatched("list:"),
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
			TestToken::newline(),
			TestToken::unmatched("-"),
			TestToken::padding(),
			TestToken::exp_b(),
			TestToken::identifier("item"),
			TestToken::exp_e(),
			TestToken::unmatched("."),
			TestToken::newline(),
			TestToken::stmt_b(),
			TestToken::endfor_s(),
			TestToken::stmt_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, macro) {
		auto src =
			"{\% macro(say_hello,prefix=\"----\") %}\n"
			"{{prefix}}{{say_hello}}\n"
			"{\% endmacro %}\n";
		auto tokens = simplify_tokens(tokenize(src));
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
			TestToken::newline(),
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
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}

	TEST(tokenizer, include) {
		auto src =
			"{\%include\"test.cpp\"%}\n";
		auto tokens = simplify_tokens(tokenize(src));
		auto expected = std::vector{
			TestToken::stmt_b(),
			TestToken::include_s(),
			TestToken::str_const("test.cpp"),
			TestToken::stmt_e(),
			TestToken::newline(),
			TestToken::eof(),
		};
		EXPECT_TOKEN(tokens, expected);
	}
}
