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
				_gen = TemplGen::create().value();
				if (_should_simplify) _gen.cfg().simplify();
			}

			TemplGenTest(TemplGenTest const &other) = delete;

			static size_t variant_count() { return 2; }

			util::Result<std::string, KError> gen(
				std::string const &src,
				TemplDict const &args)
			{
				return _gen.codegen(src, args, _test.suite_name + "-" + _test.test_name);
			}

			util::Result<std::string, KError> gen(
				std::string const &src,
				TemplObj const &args)
			{
				return _gen.codegen(src, args, _test.suite_name + "-" + _test.test_name);
			}

		private:
			Test _test;
			TemplGen _gen;
			bool _should_simplify = true;
	};

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
			"Shopping list\n"
			"- apple\n"
			"- pears\n"
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
			"foo\n"
			"bar\n"
		);

		args["add_bar"] = false;

		EXPECT_CG(
			"foo\n"
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
			"reee\n"
			"yes\n"
		);

		args["value"] = false;

		EXPECT_CG(
			"reee\n"
			"no\n"
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
			"bar\n"
		);
	}

	TEST_F(TemplGenTest, elif_chain) {
		auto src =
			"Do robots dream of eletric sheep?\n"
			"{\% if has_yes %}"
			"Most indefinitely.\n"
			"{\% elif has_no %}"
			"Of course not.\n"
			"{\% elif has_maybe %}"
			"Maybe its the electric sheep dreaming.\n"
			"{\% else %}"
			"I need sleep.\n"
			"{\% endif %}"
			"";

		auto args = TemplDict{
			{"has_yes", true},
			{"has_no", true},
			{"has_maybe", true},
		};

		EXPECT_CG(
			"Do robots dream of eletric sheep?\n"
			"Most indefinitely.\n"
		);

		args["has_yes"] = false;
		EXPECT_CG(
			"Do robots dream of eletric sheep?\n"
			"Of course not.\n"
		);

		args["has_no"] = false;
		EXPECT_CG(
			"Do robots dream of eletric sheep?\n"
			"Maybe its the electric sheep dreaming.\n"
		);

		args["has_maybe"] = false;
		EXPECT_CG(
			"Do robots dream of eletric sheep?\n"
			"I need sleep.\n"
		);
	}

	TEST_F(TemplGenTest, member_access) {
		auto src =
			"Hello I am {{person.first_name}} {{ person . last_name }} and I am {{person\n.age}} years old.";

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
			"Hello I am John Doe and I am 26 years old."
		);
	}

	TEST_F(TemplGenTest, callable) {
		auto src = "Hello {{get_name()}}\n";

		auto args = TemplObj{
			{"get_name", [](TemplList args) { return TemplObj("Jared"); }}
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
					{ "get_self", get_date }
				}
			}
		};


		auto args = TemplObj{
			{ "date", date_obj },
			{ "get_date", get_date },
		}.dict().value();

		auto src =
			"The current second is {{date.time.seconds}}s";

		EXPECT_CG(
			"The current second is 57s"
		);

		src = "The minute is {{get_date().time.get_self().time.minutes}}m";

		EXPECT_CG(
			"The minute is 13m"
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

		auto src = "User id is {{combine_str(name, id)}}";

		EXPECT_CG(
			"User id is Bob59"
		);

		src = "User id is {{combine_str(name, name)}}";
		EXPECT_KERROR(
			f.gen(src, args),
			KError::Type::CODEGEN
		);

		src = "User id is {{combine_str(id, id)}}";
		EXPECT_KERROR(
			f.gen(src, args),
			KError::Type::CODEGEN
		);

		src = "User id is {{combine_str(name)}}";
		EXPECT_KERROR(
			f.gen(src, args),
			KError::Type::CODEGEN
		);

		src = "User id is {{combine_str(name, id, id)}}";
		EXPECT_KERROR(
			f.gen(src, args),
			KError::Type::CODEGEN
		);
	}

	TEST_F(TemplGenTest, int_constant) {
		auto args = TemplDict();

		auto src = "The number is {{5}}!";
		EXPECT_CG(
			"The number is 5!"
		);

		src = "Another number is {{ 49102 }}!";
		EXPECT_CG(
			"Another number is 49102!"
		);
	}

	TEST_F(TemplGenTest, exp2) {
		auto args = TemplObj{
			{"value", 59},
			{"zero", 0},
			{"neg_value", -84},
		}.dict().value();

		auto src = "Hello {{+value}}";
		EXPECT_CG(
			"Hello 59"
		);

		src = "Hello2 {{ -value }}";
		EXPECT_CG(
			"Hello2 -59"
		);

		src = "zero: {{ -zero }}, raw: {{583}}";
		EXPECT_CG(
			"zero: 0, raw: 583"
		);

		// I don't fully know what + does.
		src = "pos_value: {{ +neg_value }}";
		EXPECT_CG(
			"pos_value: -84"
		);

		src = "bool_value: {{ false }}, {{ true }}";
		EXPECT_CG(
			"bool_value: <false>, <true>"
		);

		src = "bool_value2: {{ !false }}, {{ !true }}";
		EXPECT_CG(
			"bool_value2: <true>, <false>"
		);
	}

	TEST_F(TemplGenTest, exp3) {
		auto args = TemplObj{
			{"pos_value", 5},
			{"neg_value", -10}
		}.dict().value();

		auto src = "4*2 is {{4*2}}";
		EXPECT_CG(
			"4*2 is 8"
		);

		src = "-3*9 is {{ - 3 * 9 }}";
		EXPECT_CG(
			"-3*9 is -27"
		);

		src = "5 * -10 is {{ pos_value * neg_value }}";
		EXPECT_CG(
			"5 * -10 is -50"
		);

		src = "5 * 10 is {{ pos_value * -neg_value }}";
		EXPECT_CG(
			"5 * 10 is 50"
		);

		src = "200 / 10 is {{ 200/-neg_value }}";
		EXPECT_CG(
			"200 / 10 is 20"
		);

		src = "7 % 3 is {{7%3}}";
		EXPECT_CG(
			"7 % 3 is 1"
		);

		src = "5*10/2 is {{5*-neg_value/2}}";
		EXPECT_CG(
			"5*10/2 is 25"
		);
	}

	TEST_F(TemplGenTest, exp4) {
		auto args = TemplObj{
			{"pos_value", 5},
			{"neg_value", -10}
		}.dict().value();

		auto src = "3+12 is {{3+12}}";
		EXPECT_CG(
			"3+12 is 15"
		);

		src = "2-5 is {{2-pos_value}}";
		EXPECT_CG(
			"2-5 is -3"
		);

		src = "-12 + 2*10 is {{ -12 + 2*10}}";
		EXPECT_CG(
			"-12 + 2*10 is 8"
		);
	}

	TEST_F(TemplGenTest, exp6) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8}
		}.dict().value();

		/* greater than */
		auto src = "4 > -8 is {{4 > neg_value}}";
		EXPECT_CG(
			"4 > -8 is <true>"
		);

		src = "4 > -8 + 12 is {{4 > neg_value + 12}}";
		EXPECT_CG(
			"4 > -8 + 12 is <false>"
		);

		src = "4 > 6 is {{4 > pos_value}}";
		EXPECT_CG(
			"4 > 6 is <false>"
		);

		/* Greater than or equal */
		src = "4 >= -8 is {{4 >= neg_value}}";
		EXPECT_CG(
			"4 >= -8 is <true>"
		);

		src = "4 >= -8 + 12 is {{4 >= neg_value + 12}}";
		EXPECT_CG(
			"4 >= -8 + 12 is <true>"
		);

		src = "4 >= 6 is {{4 >= pos_value}}";
		EXPECT_CG(
			"4 >= 6 is <false>"
		);

		/* less than */
		src = "4 < -8 is {{4 < neg_value}}";
		EXPECT_CG(
			"4 < -8 is <false>"
		);

		src = "4 < -8 + 12 is {{4 < neg_value + 12}}";
		EXPECT_CG(
			"4 < -8 + 12 is <false>"
		);

		src = "4 < 6 is {{4 < pos_value}}";
		EXPECT_CG(
			"4 < 6 is <true>"
		);

		src = "4 <= -8 is {{4 <= neg_value}}";
		EXPECT_CG(
			"4 <= -8 is <false>"
		);

		src = "4 <= -8 + 12 is {{4 <= neg_value + 12}}";
		EXPECT_CG(
			"4 <= -8 + 12 is <true>"
		);

		src = "4 <= 6 is {{4 <= pos_value}}";
		EXPECT_CG(
			"4 <= 6 is <true>"
		);
	}

	TEST_F(TemplGenTest, exp7) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8}
		}.dict().value();

		auto src = "6 == 6 is {{6 == pos_value}}";
		EXPECT_CG(
			"6 == 6 is <true>"
		);

		src = "-8 == 6 is {{neg_value == pos_value}}";
		EXPECT_CG(
			"-8 == 6 is <false>"
		);
		
		src = "6 == 8 - 2 is {{pos_value == -neg_value-2}}";
		EXPECT_CG(
			"6 == 8 - 2 is <true>"
		);

		src = "6 == 8 - 2 is {{pos_value == -neg_value-2}}";
		EXPECT_CG(
			"6 == 8 - 2 is <true>"
		);

		src = "6 != 4 is {{pos_value != 4}}";
		EXPECT_CG(
			"6 != 4 is <true>"
		);

		src = "6 != -2 + 8 is {{pos_value != -2 - neg_value}}";
		EXPECT_CG(
			"6 != -2 + 8 is <false>"
		);
	}

	TEST_F(TemplGenTest, exp11) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8},
		}.dict().value();

		auto src = "true && true is {{true && true}}";
		EXPECT_CG(
			"true && true is <true>"
		);

		src = "true && false is {{true && false}}";
		EXPECT_CG(
			"true && false is <false>"
		);

		src = "6 > 0 && -8 + 9 == 1 is {{pos_value > 0 && neg_value+9 == 1}}";
		EXPECT_CG(
			"6 > 0 && -8 + 9 == 1 is <true>"
		);

		src = "6 > 0 && -8 + 9 != 1 is {{pos_value > 0 && neg_value+9 != 1}}";
		EXPECT_CG(
			"6 > 0 && -8 + 9 != 1 is <false>"
		);
	}

	TEST_F(TemplGenTest, exp12) {
		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8},
		}.dict().value();

		auto src = "true || false is {{true || false}}";
		EXPECT_CG(
			"true || false is <true>"
		);

		src = "false || false is {{false || false}}";
		EXPECT_CG(
			"false || false is <false>"
		);

		src = "6 == 0 || true && -8 + 9 == 1 is {{pos_value == 0 || true && neg_value + 9 == 1}}";
		EXPECT_CG(
			"6 == 0 || true && -8 + 9 == 1 is <true>"
		);

		src = "6 == 0 || true && -8 + 9 == 1 is {{pos_value == 0 || false && neg_value + 9 == 1}}";
		EXPECT_CG(
			"6 == 0 || true && -8 + 9 == 1 is <false>"
		);
	}

	TEST_F(TemplGenTest, str_constant) {
		auto args = TemplObj{
			{"first_name", "John"}
		}.dict().value();

		const char *src;


		src = "The expression literal is {{ \"{{\" }}";
		EXPECT_CG(
			"The expression literal is {{"
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
			"List elements:\n"
			"- 5\n"
			"- 3\n"
			"- 91\n"
			"- Totally an int\n"
			"- 2\n"
			"- -29\n"
		);

		args = TemplObj{
			{"list", {"George", "Sally", "Neo", "Peter"}},
		}.dict().value();

		EXPECT_CG(
			"List length: 4\n"
			"Is list empty: <false>\n"
			"List elements:\n"
			"- George\n"
			"- Sally\n"
			"- Neo\n"
			"- Peter\n"
		);

		args = TemplObj{
			{"list", TemplList()}
		}.dict().value();

		EXPECT_CG(
			"List length: 0\n"
			"Is list empty: <true>\n"
			"List elements:\n"
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
			"value is {{(foo+4)*bar}}";

		EXPECT_CG(
			"value is 90"
		);

		src =
			"value is {{foo+ ( 4*bar ) }}";

		EXPECT_CG(
			"value is 45"
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
			"Hello in reverse is {{hello|str_reverse}}";

		EXPECT_CG(
			"Hello in reverse is olleh"
		);

		src =
			"Hello with prepend is {{hello|str_prepend(\"hi \")}}";

		EXPECT_CG(
			"Hello with prepend is hi hello"
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
			"{{quote|indent_str}}";

		EXPECT_CG(
			"A haiku:\n"
			"\tA single raindrop,\n"
			"\tMixed with brine and oily smoke,\n"
			"\tGlistens in the web.\n"
			"\t"
		);

		src =
			"A haiku:\n"
			"{{quote|indent_str(2)}}";

		EXPECT_CG(
			"A haiku:\n"
			"\t\tA single raindrop,\n"
			"\t\tMixed with brine and oily smoke,\n"
			"\t\tGlistens in the web.\n"
			"\t\t"
		);

		src =
			"A haiku:\n"
			"{{quote|indent_str(\"--\")}}";

		EXPECT_CG(
			"A haiku:\n"
			"--A single raindrop,\n"
			"--Mixed with brine and oily smoke,\n"
			"--Glistens in the web.\n"
			"--"
		);

		src =
			"A haiku:\n"
			"{{indent_str(quote, \"> \")}}";

		EXPECT_CG(
			"A haiku:\n"
			"> A single raindrop,\n"
			"> Mixed with brine and oily smoke,\n"
			"> Glistens in the web.\n"
			"> "
		);
	}

	TEST_F(TemplGenTest, abs_filter) {
		auto args = TemplObj{
			{"value", -42},
		}.dict().value();

		auto src =
			"abs of value is {{value|abs}}";

		EXPECT_CG(
			"abs of value is 42"
		);

		src =
			"abs of expression is {{-10-2|abs}}";

		EXPECT_CG(
			"abs of expression is 8"
		);
	}

	TEST_F(TemplGenTest, capitilize) {
		auto args = TemplObj{
			{"first_name", "hezekiah"},
			{"last_name", "dombach"},
		}.dict().value();

		auto src =
			"My name is {{first_name|capitilize}} {{last_name|capitilize}}";

		EXPECT_CG(
			"My name is Hezekiah Dombach"
		);
	}

	TEST_F(TemplGenTest, center) {
		auto args = TemplObj{
			{"title_str", "title"}
		}.dict().value();

		auto src =
			"Title format is:\n"
			"{{title_str|center}}";

		EXPECT_CG(
			"Title format is:\n"
			"                                     title"
		);

		src =
			"Title format is:\n"
			"{{title_str|center(12)}}";

		EXPECT_CG(
			"Title format is:\n"
			"   title"
		);

		src =
			"Title format is:\n"
			"{{title_str|capitilize|center(12)}}";

		EXPECT_CG(
			"Title format is:\n"
			"   Title"
		);
	}

	TEST_F(TemplGenTest, first) {
		auto args = TemplObj{
			{"my_list", {"apple", "bannanna", "nuclear rod"}},
		}.dict().value();

		auto src =
			"First element of list is {{my_list|first}}";

		EXPECT_CG(
			"First element of list is apple"
		);

		args = TemplObj{
			{"my_list", TemplList()}
		}.dict().value();

		EXPECT_KERROR(f.gen(src, args), KError::CODEGEN);
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
			"    {{quote|indent}}"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"    The day has sunk\n"
			"    Into the depths of time untold.\n"
			" \n"
			"    The dawn has shattered\n"
			"    Leaving fragments strewn about.\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"   {{quote|indent(3)}}"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"   The day has sunk\n"
			"   Into the depths of time untold.\n"
			" \n"
			"   The dawn has shattered\n"
			"   Leaving fragments strewn about.\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"> {{quote|indent(\"> \")}}"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			" \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"> {{quote|indent(\"> \", false)}}"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			" \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"{{quote|indent(\"> \", true)}}"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			" \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"</quote>\n"
		);

		src =
			"<quote>\n"
			"{{quote|indent(\"> \", true, true)}}"
			"</quote>\n";

		EXPECT_CG(
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			">  \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
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
			"Hello World\n"
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
			"{{hello(\"Alex\", 423)}}"
			"";

		EXPECT_CG(
			"Hello Alex.\n"
			"Your user id is 423.\n"
		);

		src =
			"{\% macro hello(name, id) %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\")}}"
			"";

		EXPECT_KERROR(
			f.gen(src, args),
			KError::CODEGEN
		);

		src =
			"{\% macro hello(name, id) %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 142, \"Extra\")}}"
			"";

		EXPECT_KERROR(
			f.gen(src, args),
			KError::CODEGEN
		);

		src =
			"{\% macro hello(name, id, role=\"Unknown\") %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"Your role is {{role}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 142)}}"
			"";

		EXPECT_CG(
			"Hello Alex.\n"
			"Your user id is 142.\n"
			"Your role is Unknown.\n"
		);

		src =
			"{\% macro hello(name, id, role=\"Unknown\") %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"Your role is {{role}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 142, \"Admin\")}}"
			"";

		EXPECT_CG(
			"Hello Alex.\n"
			"Your user id is 142.\n"
			"Your role is Admin.\n"
		);

		src =
			"{\% macro hello(name, id, role=\"Unknown\") %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"Your role is {{role}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\")}}"
			"";

		EXPECT_KERROR(
			f.gen(src, args),
			KError::CODEGEN
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
			"Here be the list.\n"
			"----\n"
			"element: 152\n"
			"index: 1\n"
			"index0: 0\n"
			"first: <true>\n"
			"last: <false>\n"
			"----\n"
			"element: apple\n"
			"index: 2\n"
			"index0: 1\n"
			"first: <false>\n"
			"last: <false>\n"
			"----\n"
			"element: -3\n"
			"index: 3\n"
			"index0: 2\n"
			"first: <false>\n"
			"last: <false>\n"
			"----\n"
			"element: hash map\n"
			"index: 4\n"
			"index0: 3\n"
			"first: <false>\n"
			"last: <true>\n"
		);
	}

	TEST_F(TemplGenTest, macro_import) {
		auto args = TemplDict{};

		auto src =
			"{\% include \"assets/tests/common_include.cg\" %}"
			"{{say_hello()}}\n"
			"{{wrap(\"reee\")}}\n";

		EXPECT_CG(
			"Common Header\n"
			"\n"
			"\n"
			"Hello World\n"
			"\n"
			"< reee >\n"
			"\n"
		);
	}
}
