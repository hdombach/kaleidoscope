#include "Error.hpp"
#include "tests/Test.hpp"
#include "TemplGen.hpp"
#include "TemplObj.hpp"

namespace cg {
	#define EXPECT_CG(expect_src)\
		EXPECT_EQ(\
			f.gen(src, args).value(),\
			expect_src\
		);

	class TemplGenTest: public TestFixture {
		public:
			TemplGenTest(Test &test, size_t variant):
				TestFixture(test),
				_should_simplify(variant)
			{
			}

			TemplGenTest(TemplGenTest const &other) = delete;

			static size_t variant_count() { return 1; }

			util::Result<std::string, Error> gen(
				std::string const &src,
				TemplDict const &args)
			{
				count++;
				return TemplGen::codegen(src, args, _test.suite_name + "-" + _test.test_name + "-" + std::to_string(count));
			}

			util::Result<std::string, Error> gen(
				std::string const &src,
				TemplObj const &args)
			{
				count++;
				return TemplGen::codegen(src, args, _test.suite_name + "-" + _test.test_name + "-" + std::to_string(count));
			}

		private:
			Test _test;
			bool _should_simplify = true;
			int count=0;
	};

	TEST_F(TemplGenTest, nothing) {
		auto src =
			"nothing\n";

		auto args = TemplDict{};

		EXPECT_CG(
			"nothing\n"
		);
	}

	TEST_F(TemplGenTest, expressions) {
		auto src =
			"Hello everyone\n"
			"My name is {{ name }}!\n";

		auto args = TemplDict{
			{"name", "Hezekiah Dombach"}
		};

		EXPECT_CG(
			"Hello everyone\n"
			"My name is Hezekiah Dombach!\n"
		);
	}

	TEST_F(TemplGenTest, forloop) {
		auto src =
			"Shopping list\n"
			"{\% for item in shopping_list %}\n"
			"- {{ item }}\n"
			"{\% endfor %}\n"
			"";

		auto args = TemplDict{
			{"shopping_list", {
					"apple",
					"pears",
				}
			}
		};

		EXPECT_CG(
			"Shopping list\n\n"
			"- apple\n\n"
			"- pears\n\n"
		);
	}

	TEST_F(TemplGenTest, if) {
		auto src =
			"foo\n"
			"{\% if add_bar %}\n"
			"bar\n"
			"{\% endif %}\n"
			"";

		auto args = TemplDict{
			{"add_bar", true}
		};

		EXPECT_CG(
			"foo\n\n"
			"bar\n\n"
		);

		args["add_bar"] = false;

		EXPECT_CG(
			"foo\n\n"
		);
	}

	TEST_F(TemplGenTest, if_else) {
		auto src =
			"reee\n"
			"{\% if value %}\n"
			"yes\n"
			"{\% else %}\n"
			"no\n"
			"{\% endif %}\n"
			"";

		auto args = TemplDict{
			{"value", true}
		};

		EXPECT_CG(
			"reee\n\n"
			"yes\n\n"
		);

		args["value"] = false;

		EXPECT_CG(
			"reee\n\n"
			"no\n\n"
		);
	}

	TEST_F(TemplGenTest, empty_if) {
		auto src =
			"foo\n"
			"{\% if add_bar %}\n"
			"{\% endif %}\n"
			"bar\n"
			"";

		auto args = TemplDict{
			{"add_bar", true}
		};

		EXPECT_CG(
			"foo\n"
			"\n"
			"\n"
			"bar\n"
		);
	}

	TEST_F(TemplGenTest, empty_elseif) {
		auto src =
			"foo\n"
			"{\%if add_bar%}\n"
			"{\%else%}\n"
			"{\%endif%}\n"
			"bar\n"
			"";

		auto args = TemplDict{
			{"add_bar", true}
		};

		EXPECT_CG(
			"foo\n"
			"\n"
			"\n"
			"bar\n"
		);
	}

	TEST_F(TemplGenTest, empty_elif_chain) {
		auto src =
			"foo\n"
			"{\% if add_bar %}\n"
			"{\% elif add_bar2 %}\n"
			"{\% else %}\n"
			"{\% endif %}\n"
			"bar\n"
			"";

		auto args = TemplDict{
			{"add_bar", true},
			{"add_bar2", true},
		};

		EXPECT_CG(
			"foo\n"
			"\n"
			"\n"
			"bar\n"
		);

	}

	TEST_F(TemplGenTest, elif_chain) {
		auto src =
			"Do robots dream of electric sheep?\n"
			"{\% if has_yes %}\n"
			"Most indefinitely.\n"
			"{\% elif has_no %}\n"
			"Of course not.\n"
			"{\% elif has_maybe %}\n"
			"Maybe its the electric sheep dreaming.\n"
			"{\% else %}\n"
			"I need sleep.\n"
			"{\% endif %}\n"
			"";

		auto args = TemplDict{
			{"has_yes", true},
			{"has_no", true},
			{"has_maybe", true},
		};

		EXPECT_CG(
			"Do robots dream of electric sheep?\n"
			"\n"
			"Most indefinitely.\n"
			"\n"
		);

		args["has_yes"] = false;
		EXPECT_CG(
			"Do robots dream of electric sheep?\n"
			"\n"
			"Of course not.\n"
			"\n"
		);

		args["has_no"] = false;
		EXPECT_CG(
			"Do robots dream of electric sheep?\n"
			"\n"
			"Maybe its the electric sheep dreaming.\n"
			"\n"
		);

		args["has_maybe"] = false;
		EXPECT_CG(
			"Do robots dream of electric sheep?\n"
			"\n"
			"I need sleep.\n"
			"\n"
		);
	}

	TEST_F(TemplGenTest, member_access) {
		auto src =
			"Hello I am {{person.first_name}} {{ person . last_name }} and I am {{person\n.age}} years old.\n";

		auto args = TemplDict{
			{
				"person", {
					{"first_name", "John"},
					{"last_name", "Doe"},
					{"age", 26}
				}
			}
		};

		EXPECT_CG(
			"Hello I am John Doe and I am 26 years old.\n"
		);
	}

	TEST_F(TemplGenTest, callable) {
		auto src = "Hello {{get_name()}}\n";

		auto args = TemplObj{
			{"get_name", TemplFunc([](TemplList args) { return TemplObj("Jared"); })}
		};

		EXPECT_CG(
			"Hello Jared\n"
		);
	}

	TEST_F(TemplGenTest, call_member_chain) {
		auto get_abs_sec = [](TemplList l) {
			return l[0].get_attribute("seconds")->integer().value()
				+ l[0].get_attribute("minutes")->integer().value() * 60
				+ l[0].get_attribute("hours")->integer().value() * 60 * 60;
		};

		TemplObj date_obj;
		auto get_date = [&date_obj](TemplList) { return date_obj; };

		date_obj = TemplObj{
			{
				"time", {
					{"seconds", 57},
					{"minutes", 13},
					{"hours", 2},
					{ "get_self", TemplFunc(get_date) }
				}
			}
		};


		auto args = TemplObj{
			{ "date", date_obj },
			{ "get_date", TemplFunc(get_date) },
		}.dict().value();

		auto src =
			"The current second is {{date.time.seconds}}s\n";

		EXPECT_CG(
			"The current second is 57s\n"
		);

		src = "The minute is {{get_date().time.get_self().time.minutes}}m\n";

		EXPECT_CG(
			"The minute is 13m\n"
		);
	}

	TEST_F(TemplGenTest, mk_func) {
		auto combined = [](std::string name, int64_t num) -> TemplFuncRes {
			return {name + std::to_string(num)};
		};

		auto args = TemplObj{
			{"name", "Bob"},
			{"id", 59},
			{"combine_str", mk_templfunc(combined)}
		}.dict().value();

		auto src = "User id is {{combine_str(name, id)}}\n";

		EXPECT_CG(
			"User id is Bob59\n"
		);

		src = "User id is {{combine_str(name, name)}}\n";
		EXPECT_TERROR(
			f.gen(src, args),
			ErrorType::MISC
		);

		src = "User id is {{combine_str(id, id)}}\n";
		EXPECT_TERROR(
			f.gen(src, args),
			ErrorType::MISC
		);

		src = "User id is {{combine_str(name)}}\n";
		EXPECT_TERROR(
			f.gen(src, args),
			ErrorType::MISC
		);

		src = "User id is {{combine_str(name, id, id)}}\n";
		EXPECT_TERROR(
			f.gen(src, args),
			ErrorType::MISC
		);
	}

	TEST_F(TemplGenTest, int_constant) {
		auto args = TemplDict();

		auto src = "The number is {{5}}!\n";
		EXPECT_CG(
			"The number is 5!\n"
		);

		src = "Another number is {{ 49102 }}!\n";
		EXPECT_CG(
			"Another number is 49102!\n"
		);
	}

	TEST_F(TemplGenTest, exp2) {
		auto args = TemplObj{
			{"value", 59},
			{"zero", 0},
			{"neg_value", -84},
		}.dict().value();

		auto src = "Hello {{+value}}\n";
		EXPECT_CG(
			"Hello 59\n"
		);

		src = "Hello2 {{ -value }}\n";
		EXPECT_CG(
			"Hello2 -59\n"
		);

		src = "zero: {{ -zero }}, raw: {{583}}\n";
		EXPECT_CG(
			"zero: 0, raw: 583\n"
		);

		// I don't fully know what + does.
		src = "pos_value: {{ +neg_value }}\n";
		EXPECT_CG(
			"pos_value: -84\n"
		);

		src = "bool_value: {{ false }}, {{ true }}\n";
		EXPECT_CG(
			"bool_value: <false>, <true>\n"
		);

		src = "bool_value2: {{ !false }}, {{ !true }}\n";
		EXPECT_CG(
			"bool_value2: <true>, <false>\n"
		);
	}

	TEST_F(TemplGenTest, exp3) {
		auto args = TemplObj{
			{"pos_value", 5},
			{"neg_value", -10}
		}.dict().value();

		auto src = "4*2 is {{4*2}}\n";
		EXPECT_CG(
			"4*2 is 8\n"
		);

		src = "-3*9 is {{ - 3 * 9 }}\n";
		EXPECT_CG(
			"-3*9 is -27\n"
		);

		src = "5 * -10 is {{ pos_value * neg_value }}\n";
		EXPECT_CG(
			"5 * -10 is -50\n"
		);

		src = "5 * 10 is {{ pos_value * -neg_value }}\n";
		EXPECT_CG(
			"5 * 10 is 50\n"
		);

		src = "200 / 10 is {{ 200/-neg_value }}\n";
		EXPECT_CG(
			"200 / 10 is 20\n"
		);

		src = "7 % 3 is {{7%3}}\n";
		EXPECT_CG(
			"7 % 3 is 1\n"
		);

		src = "5*10/2 is {{5*-neg_value/2}}\n";
		EXPECT_CG(
			"5*10/2 is 25\n"
		);
	}

	TEST_F(TemplGenTest, exp4) {
		auto args = TemplObj{
			{"pos_value", 5},
			{"neg_value", -10}
		}.dict().value();

		auto src = "3+12 is {{3+12}}\n";
		EXPECT_CG(
			"3+12 is 15\n"
		);

		src = "2-5 is {{2-pos_value}}\n";
		EXPECT_CG(
			"2-5 is -3\n"
		);

		src = "-12 + 2*10 is {{ -12 + 2*10}}\n";
		EXPECT_CG(
			"-12 + 2*10 is 8\n"
		);
	}

	TEST_F(TemplGenTest, exp6) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8}
		}.dict().value();

		/* greater than */
		auto src = "4 > -8 is {{4 > neg_value}}\n";
		EXPECT_CG(
			"4 > -8 is <true>\n"
		);

		src = "4 > -8 + 12 is {{4 > neg_value + 12}}\n";
		EXPECT_CG(
			"4 > -8 + 12 is <false>\n"
		);

		src = "4 > 6 is {{4 > pos_value}}\n";
		EXPECT_CG(
			"4 > 6 is <false>\n"
		);

		/* Greater than or equal */
		src = "4 >= -8 is {{4 >= neg_value}}\n";
		EXPECT_CG(
			"4 >= -8 is <true>\n"
		);

		src = "4 >= -8 + 12 is {{4 >= neg_value + 12}}\n";
		EXPECT_CG(
			"4 >= -8 + 12 is <true>\n"
		);

		src = "4 >= 6 is {{4 >= pos_value}}\n";
		EXPECT_CG(
			"4 >= 6 is <false>\n"
		);

		/* less than */
		src = "4 < -8 is {{4 < neg_value}}\n";
		EXPECT_CG(
			"4 < -8 is <false>\n"
		);

		src = "4 < -8 + 12 is {{4 < neg_value + 12}}\n";
		EXPECT_CG(
			"4 < -8 + 12 is <false>\n"
		);

		src = "4 < 6 is {{4 < pos_value}}\n";
		EXPECT_CG(
			"4 < 6 is <true>\n"
		);

		src = "4 <= -8 is {{4 <= neg_value}}\n";
		EXPECT_CG(
			"4 <= -8 is <false>\n"
		);

		src = "4 <= -8 + 12 is {{4 <= neg_value + 12}}\n";
		EXPECT_CG(
			"4 <= -8 + 12 is <true>\n"
		);

		src = "4 <= 6 is {{4 <= pos_value}}\n";
		EXPECT_CG(
			"4 <= 6 is <true>\n"
		);
	}

	TEST_F(TemplGenTest, exp7) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8}
		}.dict().value();

		auto src = "6 == 6 is {{6 == pos_value}}\n";
		EXPECT_CG(
			"6 == 6 is <true>\n"
		);

		src = "-8 == 6 is {{neg_value == pos_value}}\n";
		EXPECT_CG(
			"-8 == 6 is <false>\n"
		);
		
		src = "6 == 8 - 2 is {{pos_value == -neg_value-2}}\n";
		EXPECT_CG(
			"6 == 8 - 2 is <true>\n"
		);

		src = "6 == 8 - 2 is {{pos_value == -neg_value-2}}\n";
		EXPECT_CG(
			"6 == 8 - 2 is <true>\n"
		);

		src = "6 != 4 is {{pos_value != 4}}\n";
		EXPECT_CG(
			"6 != 4 is <true>\n"
		);

		src = "6 != -2 + 8 is {{pos_value != -2 - neg_value}}\n";
		EXPECT_CG(
			"6 != -2 + 8 is <false>\n"
		);
	}

	TEST_F(TemplGenTest, exp11) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8},
		}.dict().value();

		auto src = "true && true is {{true && true}}\n";
		EXPECT_CG(
			"true && true is <true>\n"
		);

		src = "true && false is {{true && false}}\n";
		EXPECT_CG(
			"true && false is <false>\n"
		);

		src = "6 > 0 && -8 + 9 == 1 is {{pos_value > 0 && neg_value+9 == 1}}\n";
		EXPECT_CG(
			"6 > 0 && -8 + 9 == 1 is <true>\n"
		);

		src = "6 > 0 && -8 + 9 != 1 is {{pos_value > 0 && neg_value+9 != 1}}\n";
		EXPECT_CG(
			"6 > 0 && -8 + 9 != 1 is <false>\n"
		);
	}

	TEST_F(TemplGenTest, exp12) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8},
		}.dict().value();

		auto src = "true || false is {{true || false}}\n";
		EXPECT_CG(
			"true || false is <true>\n"
		);

		src = "false || false is {{false || false}}\n";
		EXPECT_CG(
			"false || false is <false>\n"
		);

		src = "6 == 0 || true && -8 + 9 == 1 is {{pos_value == 0 || true && neg_value + 9 == 1}}\n";
		EXPECT_CG(
			"6 == 0 || true && -8 + 9 == 1 is <true>\n"
		);

		src = "6 == 0 || true && -8 + 9 == 1 is {{pos_value == 0 || false && neg_value + 9 == 1}}\n";
		EXPECT_CG(
			"6 == 0 || true && -8 + 9 == 1 is <false>\n"
		);
	}

	TEST_F(TemplGenTest, str_constant) {
		auto args = TemplObj{
			{"first_name", "John"}
		}.dict().value();

		const char *src;


		src = "The expression literal is {{ \"{{\" }}\n";
		EXPECT_CG(
			"The expression literal is {{\n"
		);
	}

	TEST_F(TemplGenTest, list_builtins) {
		auto src =
			"List length: {{list.length()}}\n"
			"Is list empty: {{list.empty()}}\n"
			"List elements:\n"
			"{\% for element in list %}\n"
			"- {{element}}\n"
			"{\% endfor %}\n";

		auto args = TemplObj{
			{"list", {5, 3, 91, "Totally an int", 2, -29}}
		}.dict().value();

		EXPECT_CG(
			"List length: 6\n"
			"Is list empty: <false>\n"
			"List elements:\n\n"
			"- 5\n\n"
			"- 3\n\n"
			"- 91\n\n"
			"- Totally an int\n\n"
			"- 2\n\n"
			"- -29\n\n"
		);

		args = TemplObj{
			{"list", {"George", "Sally", "Neo", "Peter"}},
		}.dict().value();

		EXPECT_CG(
			"List length: 4\n"
			"Is list empty: <false>\n"
			"List elements:\n\n"
			"- George\n\n"
			"- Sally\n\n"
			"- Neo\n\n"
			"- Peter\n\n"
		);

		args = TemplObj{
			{"list", TemplList()}
		}.dict().value();

		EXPECT_CG(
			"List length: 0\n"
			"Is list empty: <true>\n"
			"List elements:\n\n"
		);
	}

	TEST_F(TemplGenTest, str_builtins) {
		auto args = TemplObj{
			{"str", "Hello World"},
		}.dict().value();

		auto src =
			"string: {{str}}\n"
			"size: {{str.length()}}\n"
			"empty: {{str.empty()}}\n"
			"upper: {{str.upper()}}\n"
			"lower: {{str.lower()}}\n";

		EXPECT_CG(
			"string: Hello World\n"
			"size: 11\n"
			"empty: <false>\n"
			"upper: HELLO WORLD\n"
			"lower: hello world\n"
		);
	}

	TEST_F(TemplGenTest, paranthesis) {
		auto args = TemplObj{
			{"foo", 5},
			{"bar", 10}
		}.dict().value();

		auto src = 
			"value is {{(foo+4)*bar}}\n";

		EXPECT_CG(
			"value is 90\n"
		);

		src =
			"value is {{foo+ ( 4*bar ) }}\n";

		EXPECT_CG(
			"value is 45\n"
		);
	}

	TEST_F(TemplGenTest, filter) {
		auto str_reverse = [](std::string str) -> TemplFuncRes {
			auto res = str;
			std::reverse(res.begin(), res.end());
			return {res};
		};

		auto str_prepend = [](std::string str, std::string begin) -> TemplFuncRes {
			return {begin + str};
		};

		auto args = TemplObj{
			{"str_reverse", mk_templfunc(str_reverse)},
			{"str_prepend", mk_templfunc(str_prepend)},
			{"hello", "hello"}
		}.dict().value();

		auto src =
			"Hello in reverse is {{hello|str_reverse}}\n";

		EXPECT_CG(
			"Hello in reverse is olleh\n"
		);

		src =
			"Hello with prepend is {{hello|str_prepend(\"hi \")}}\n";

		EXPECT_CG(
			"Hello with prepend is hi hello\n"
		);
	}

	TEST_F(TemplGenTest, overloaded_functions) {
		auto indent_str_str = [](std::string s, std::string indent) -> TemplFuncRes {
			auto r = indent;
			for (auto c : s) {
				r += c;
				if (c == '\n') {
					r += indent;
				}
			}
			return {r};
		};

		auto indent_str = [indent_str_str](std::string s) -> TemplFuncRes {
			return indent_str_str(s, "\t");
		};

		auto indent_str_int = [indent_str_str](std::string s, TemplInt indent) -> TemplFuncRes {
			auto indent_str = std::string();
			for (auto i = 0; i < indent; i++) {
				indent_str += "\t";
			}
			return indent_str_str(s, indent_str);
		};

		auto indent = mk_templfuncs(indent_str_str, indent_str, indent_str_int);

		auto args = TemplObj{
			{"indent_str", indent},
			{"quote",
				"A single raindrop,\n"
				"Mixed with brine and oily smoke,\n"
				"Glistens in the web.\n"
			}
		}.dict().value();

		auto src =
			"A haiku:\n"
			"{{quote|indent_str}}\n";

		EXPECT_CG(
			"A haiku:\n"
			"\tA single raindrop,\n"
			"\tMixed with brine and oily smoke,\n"
			"\tGlistens in the web.\n"
			"\t\n"
		);

		src =
			"A haiku:\n"
			"{{quote|indent_str(2)}}\n";

		EXPECT_CG(
			"A haiku:\n"
			"\t\tA single raindrop,\n"
			"\t\tMixed with brine and oily smoke,\n"
			"\t\tGlistens in the web.\n"
			"\t\t\n"
		);

		src =
			"A haiku:\n"
			"{{quote|indent_str(\"--\")}}\n";

		EXPECT_CG(
			"A haiku:\n"
			"--A single raindrop,\n"
			"--Mixed with brine and oily smoke,\n"
			"--Glistens in the web.\n"
			"--\n"
		);

		src =
			"A haiku:\n"
			"{{indent_str(quote, \"> \")}}\n";

		EXPECT_CG(
			"A haiku:\n"
			"> A single raindrop,\n"
			"> Mixed with brine and oily smoke,\n"
			"> Glistens in the web.\n"
			"> \n"
		);
	}

	TEST_F(TemplGenTest, abs_filter) {
		auto args = TemplObj{
			{"v", -42},
		}.dict().value();

		auto src =
			"abs of value is {{v|abs}}\n";

		EXPECT_CG(
			"abs of value is 42\n"
		);

		src =
			"abs of expression is {{-10-2|abs}}\n";

		EXPECT_CG(
			"abs of expression is 8\n"
		);
	}

	TEST_F(TemplGenTest, capitilize) {
		auto args = TemplObj{
			{"first_name", "hezekiah"},
			{"last_name", "dombach"},
		}.dict().value();

		auto src =
			"My name is {{first_name|capitilize}} {{last_name|capitilize}}\n";

		EXPECT_CG(
			"My name is Hezekiah Dombach\n"
		);
	}

	TEST_F(TemplGenTest, center) {
		auto args = TemplObj{
			{"title_str", "title"}
		}.dict().value();

		auto src =
			"Title format is:\n"
			"{{title_str|center}}\n";

		EXPECT_CG(
			"Title format is:\n"
			"                                     title\n"
		);

		src =
			"Title format is:\n"
			"{{title_str|center(12)}}\n";

		EXPECT_CG(
			"Title format is:\n"
			"   title\n"
		);

		src =
			"Title format is:\n"
			"{{title_str|capitilize|center(12)}}\n";

		EXPECT_CG(
			"Title format is:\n"
			"   Title\n"
		);
	}

	TEST_F(TemplGenTest, first) {
		auto args = TemplObj{
			{"my_list", {"apple", "bannanna", "nuclear rod"}},
		}.dict().value();

		auto src =
			"First element of list is {{my_list|first}}\n";

		EXPECT_CG(
			"First element of list is apple\n"
		);

		args = TemplObj{
			{"my_list", TemplList()}
		}.dict().value();

		EXPECT_TERROR(f.gen(src, args), ErrorType::MISC);
	}

	TEST_F(TemplGenTest, indent) {
		auto args = TemplObj{
			{"quote",
				"The day has sunk\n"
				"Into the depths of time untold.\n"
				" \n"
				"The dawn has shattered\n"
				"Leaving fragments strewn about.\n"
			}
		}.dict().value();

		auto src =
			"<quote>\n"
			"    {{quote|indent}}\n"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"    The day has sunk\n"
			"    Into the depths of time untold.\n"
			" \n"
			"    The dawn has shattered\n"
			"    Leaving fragments strewn about.\n"
			"\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"   {{quote|indent(3)}}\n"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"   The day has sunk\n"
			"   Into the depths of time untold.\n"
			" \n"
			"   The dawn has shattered\n"
			"   Leaving fragments strewn about.\n"
			"\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"> {{quote|indent(\"> \")}}\n"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			" \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"> {{quote|indent(\"> \", false)}}\n"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			" \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"{{quote|indent(\"> \", true)}}\n"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			" \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"{{quote|indent(\"> \", true, true)}}\n"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			">  \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"\n"
			"</quote>\n"
		);
	}

	TEST_F(TemplGenTest, macro) {
		auto args = TemplDict();

		auto src =
			"{\% macro hello() %}\n"
			"Hello World"
			"{\% endmacro %}\n"
			"{{hello()}}\n"
			"{{hello()}}\n";

		EXPECT_CG(
			"\n\n"
			"Hello World\n\n"
			"Hello World\n"
		);
	}

	TEST_F(TemplGenTest, macro_w_args) {
		auto args = TemplDict();

		auto src =
			"{\% macro hello(name, id) %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 423)}}\n"
			"";

		EXPECT_CG(
			"\n\n"
			"Hello Alex.\n"
			"Your user id is 423.\n\n"
		);

		src =
			"{\% macro hello(name, id) %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\")}}\n"
			"";

		EXPECT_TERROR(
			f.gen(src, args),
			ErrorType::MISC
		);

		src =
			"{\% macro hello(name, id) %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 142, \"Extra\")}}\n"
			"";

		EXPECT_TERROR(
			f.gen(src, args),
			ErrorType::MISC
		);

		src =
			"{\% macro hello(name, id, role=\"Unknown\") %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"Your role is {{role}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 142)}}\n"
			"";

		EXPECT_CG(
			"\n\n"
			"Hello Alex.\n"
			"Your user id is 142.\n"
			"Your role is Unknown.\n\n"
		);

		src =
			"{\% macro hello(name, id, role=\"Unknown\") %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"Your role is {{role}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 142, \"Admin\")}}\n"
			"";

		EXPECT_CG(
			"\n\n"
			"Hello Alex.\n"
			"Your user id is 142.\n"
			"Your role is Admin.\n\n"
		);

		src =
			"{\% macro hello(name, id, role=\"Unknown\") %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"Your role is {{role}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\")}}\n"
			"";

		EXPECT_TERROR(
			f.gen(src, args),
			ErrorType::MISC
		);
	}

	TEST_F(TemplGenTest, for_loop_var) {
		auto args = TemplObj{
			{"list", {152, "apple", -3, "hash map"}}
		};

		auto src =
			"Here be the list.\n"
			"{\%for element in list %}\n"
			"----\n"
			"element: {{element}}\n"
			"index: {{loop.index}}\n"
			"index0: {{loop.index0}}\n"
			"first: {{loop.first}}\n"
			"last: {{loop.last}}\n"
			"{\% endfor %}\n"
			"";

		EXPECT_CG(
			"Here be the list.\n\n"
			"----\n"
			"element: 152\n"
			"index: 1\n"
			"index0: 0\n"
			"first: <true>\n"
			"last: <false>\n\n"
			"----\n"
			"element: apple\n"
			"index: 2\n"
			"index0: 1\n"
			"first: <false>\n"
			"last: <false>\n\n"
			"----\n"
			"element: -3\n"
			"index: 3\n"
			"index0: 2\n"
			"first: <false>\n"
			"last: <false>\n\n"
			"----\n"
			"element: hash map\n"
			"index: 4\n"
			"index0: 3\n"
			"first: <false>\n"
			"last: <true>\n\n"
		);
	}

	TEST_F(TemplGenTest, macro_import) {
		auto args = TemplDict{};

		auto src =
			"{\% include \"assets/tests/common_include.cg\" %}\n"
			"{{say_hello()}}\n"
			"{{wrap(\"reee\")}}\n";

		EXPECT_CG(
			"Common Header\n"
			"\n\n\n\n\n\n"
			"Hello World\n"
			"\n\n"
			"< reee >\n\n"
		);
	}
}
