#include "tests/Test.hpp"
#include "TemplGen.hpp"
#include "TemplObj.hpp"

namespace cg {
	TEST(templ_gen, expressions) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto src =
			"Hello everyone\n"
			"My name is {{ name }}!\n";

		auto args = TemplDict{
			{"name", "Hezekiah Dombach"}
		};

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Hello everyone\n"
			"My name is Hezekiah Dombach!\n"
		);
	}

	TEST(templ_gen, forloop) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Shopping list\n"
			"- apple\n"
			"- pears\n"
		);
	}

	TEST(templ_gen, if) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto src =
			"foo\n"
			"{\% if add_bar %}\n"
			"bar\n"
			"{\% endif %}\n"
			"";

		auto args = TemplDict{
			{"add_bar", true}
		};

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"foo\n"
			"bar\n"
		);

		args["add_bar"] = false;

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"foo\n"
		);
	}

	TEST(templ_gen, if_else) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"reee\n"
			"yes\n"
		);

		args["value"] = false;

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"reee\n"
			"no\n"
		);
	}

	TEST(templ_gen, empty_if) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto src =
			"foo\n"
			"{\% if add_bar %}\n"
			"{\% endif %}\n"
			"bar\n"
			"";

		auto args = TemplDict{
			{"add_bar", true}
		};

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"foo\n"
			"bar\n"
		);
	}

	TEST(templ_gen, empty_elseif) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"foo\n"
			"bar\n"
		);
	}

	TEST(templ_gen, elif_chain) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Do robots dream of eletric sheep?\n"
			"Most indefinitely.\n"
		);

		args["has_yes"] = false;
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Do robots dream of eletric sheep?\n"
			"Of course not.\n"
		);

		args["has_no"] = false;
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Do robots dream of eletric sheep?\n"
			"Maybe its the electric sheep dreaming.\n"
		);

		args["has_maybe"] = false;
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Do robots dream of eletric sheep?\n"
			"I need sleep.\n"
		);
	}

	TEST(templ_gen, member_access) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Hello I am John Doe and I am 26 years old."
		);
	}

	TEST(templ_gen, callable) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto src = "Hello {{get_name()}}\n";

		auto args = TemplObj{
			{"get_name", [](TemplList args) { return TemplObj("Jared"); }}
		};

		EXPECT_EQ(
			gen->codegen(src, args.dict().value()).value(),
			"Hello Jared\n"
		);
	}

	TEST(templ_gen, call_member_chain) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"The current second is 57s"
		);

		src = "The minute is {{get_date().time.get_self().time.minutes}}m";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"The minute is 13m"
		);
	}

	TEST(templ_gen, mk_func) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto combined = [](std::string name, int64_t num) -> TemplFuncRes {
			return {name + std::to_string(num)};
		};

		auto args = TemplObj{
			{"name", "Bob"},
			{"id", 59},
			{"combine_str", mk_templfunc(combined)}
		}.dict().value();

		auto src = "User id is {{combine_str(name, id)}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"User id is Bob59"
		);

		src = "User id is {{combine_str(name, name)}}";
		EXPECT_KERROR(
			gen->codegen(src, args),
			KError::Type::CODEGEN
		);

		src = "User id is {{combine_str(id, id)}}";
		EXPECT_KERROR(
			gen->codegen(src, args),
			KError::Type::CODEGEN
		);

		src = "User id is {{combine_str(name)}}";
		EXPECT_KERROR(
			gen->codegen(src, args),
			KError::Type::CODEGEN
		);

		src = "User id is {{combine_str(name, id, id)}}";
		EXPECT_KERROR(
			gen->codegen(src, args),
			KError::Type::CODEGEN
		);
	}

	TEST(templ_gen, int_constant) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto src = "The number is {{5}}!";
		EXPECT_EQ(
			gen->codegen(src, TemplDict()).value(),
			"The number is 5!"
		);

		src = "Another number is {{ 49102 }}!";
		EXPECT_EQ(
			gen->codegen(src, TemplDict()).value(),
			"Another number is 49102!"
		);
	}

	TEST(templ_gen, exp2) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"value", 59},
			{"zero", 0},
			{"neg_value", -84},
		}.dict().value();

		auto src = "Hello {{+value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Hello 59"
		);

		src = "Hello2 {{ -value }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Hello2 -59"
		);

		src = "zero: {{ -zero }}, raw: {{583}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"zero: 0, raw: 583"
		);

		// I don't fully know what + does.
		src = "pos_value: {{ +neg_value }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"pos_value: -84"
		);

		src = "bool_value: {{ false }}, {{ true }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"bool_value: <false>, <true>"
		);

		src = "bool_value2: {{ !false }}, {{ !true }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"bool_value2: <true>, <false>"
		);
	}

	TEST(templ_gen, exp3) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"pos_value", 5},
			{"neg_value", -10}
		}.dict().value();

		auto src = "4*2 is {{4*2}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4*2 is 8"
		);

		src = "-3*9 is {{ - 3 * 9 }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"-3*9 is -27"
		);

		src = "5 * -10 is {{ pos_value * neg_value }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"5 * -10 is -50"
		);

		src = "5 * 10 is {{ pos_value * -neg_value }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"5 * 10 is 50"
		);

		src = "200 / 10 is {{ 200/-neg_value }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"200 / 10 is 20"
		);

		src = "7 % 3 is {{7%3}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"7 % 3 is 1"
		);

		src = "5*10/2 is {{5*-neg_value/2}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"5*10/2 is 25"
		);
	}

	TEST(templ_gen, exp4) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"pos_value", 5},
			{"neg_value", -10}
		}.dict().value();

		auto src = "3+12 is {{3+12}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"3+12 is 15"
		);

		src = "2-5 is {{2-pos_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"2-5 is -3"
		);

		src = "-12 + 2*10 is {{ -12 + 2*10}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"-12 + 2*10 is 8"
		);
	}

	TEST(templ_gen, exp6) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8}
		}.dict().value();

		/* greater than */
		auto src = "4 > -8 is {{4 > neg_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 > -8 is <true>"
		);

		src = "4 > -8 + 12 is {{4 > neg_value + 12}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 > -8 + 12 is <false>"
		);

		src = "4 > 6 is {{4 > pos_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 > 6 is <false>"
		);

		/* Greater than or equal */
		src = "4 >= -8 is {{4 >= neg_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 >= -8 is <true>"
		);

		src = "4 >= -8 + 12 is {{4 >= neg_value + 12}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 >= -8 + 12 is <true>"
		);

		src = "4 >= 6 is {{4 >= pos_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 >= 6 is <false>"
		);

		/* less than */
		src = "4 < -8 is {{4 < neg_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 < -8 is <false>"
		);

		src = "4 < -8 + 12 is {{4 < neg_value + 12}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 < -8 + 12 is <false>"
		);

		src = "4 < 6 is {{4 < pos_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 < 6 is <true>"
		);

		src = "4 <= -8 is {{4 <= neg_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 <= -8 is <false>"
		);

		src = "4 <= -8 + 12 is {{4 <= neg_value + 12}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 <= -8 + 12 is <true>"
		);

		src = "4 <= 6 is {{4 <= pos_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"4 <= 6 is <true>"
		);
	}

	TEST(templ_gen, exp7) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8}
		}.dict().value();

		auto src = "6 == 6 is {{6 == pos_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 == 6 is <true>"
		);

		src = "-8 == 6 is {{neg_value == pos_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"-8 == 6 is <false>"
		);
		
		src = "6 == 8 - 2 is {{pos_value == -neg_value-2}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 == 8 - 2 is <true>"
		);

		src = "6 == 8 - 2 is {{pos_value == -neg_value-2}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 == 8 - 2 is <true>"
		);

		src = "6 != 4 is {{pos_value != 4}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 != 4 is <true>"
		);

		src = "6 != -2 + 8 is {{pos_value != -2 - neg_value}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 != -2 + 8 is <false>"
		);
	}

	TEST(templ_gen, exp11) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8},
		}.dict().value();

		auto src = "true && true is {{true && true}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"true && true is <true>"
		);

		src = "true && false is {{true && false}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"true && false is <false>"
		);

		src = "6 > 0 && -8 + 9 == 1 is {{pos_value > 0 && neg_value+9 == 1}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 > 0 && -8 + 9 == 1 is <true>"
		);

		src = "6 > 0 && -8 + 9 != 1 is {{pos_value > 0 && neg_value+9 != 1}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 > 0 && -8 + 9 != 1 is <false>"
		);
	}

	TEST(templ_gen, exp12) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"pos_value", 6},
			{"neg_value", -8},
		}.dict().value();

		auto src = "true || false is {{true || false}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"true || false is <true>"
		);

		src = "false || false is {{false || false}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"false || false is <false>"
		);

		src = "6 == 0 || true && -8 + 9 == 1 is {{pos_value == 0 || true && neg_value + 9 == 1}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 == 0 || true && -8 + 9 == 1 is <true>"
		);

		src = "6 == 0 || true && -8 + 9 == 1 is {{pos_value == 0 || false && neg_value + 9 == 1}}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"6 == 0 || true && -8 + 9 == 1 is <false>"
		);
	}

	TEST(templ_gen, str_constant) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"first_name", "John"}
		}.dict().value();

		auto src = "The expression literal is {{ \"{{\" }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"The expression literal is {{"
		);


		src = "My name is {{ first_name + \" Doe\" }}";
		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"My name is John Doe"
		);
	}

	TEST(templ_gen, list_builtins) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"List length: 0\n"
			"Is list empty: <true>\n"
			"List elements:\n"
		);
	}

	TEST(templ_gen, str_builtins) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"str", "Hello World"},
		}.dict().value();

		auto src =
			"string: {{str}}\n"
			"size: {{str.length()}}\n"
			"empty: {{str.empty()}}\n"
			"upper: {{str.upper()}}\n"
			"lower: {{str.lower()}}\n";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"string: Hello World\n"
			"size: 11\n"
			"empty: <false>\n"
			"upper: HELLO WORLD\n"
			"lower: hello world\n"
		);
	}

	TEST(templ_gen, paranthesis) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"foo", 5},
			{"bar", 10}
		}.dict().value();

		auto src = 
			"value is {{(foo+4)*bar}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"value is 90"
		);

		src =
			"value is {{foo+ ( 4*bar ) }}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"value is 45"
		);
	}

	TEST(templ_gen, filter) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Hello in reverse is olleh"
		);

		src =
			"Hello with prepend is {{hello|str_prepend(\"hi \")}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Hello with prepend is hi hello"
		);
	}

	TEST(templ_gen, overloaded_functions) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"A haiku:\n"
			"\tA single raindrop,\n"
			"\tMixed with brine and oily smoke,\n"
			"\tGlistens in the web.\n"
			"\t"
		);

		src =
			"A haiku:\n"
			"{{quote|indent_str(2)}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"A haiku:\n"
			"\t\tA single raindrop,\n"
			"\t\tMixed with brine and oily smoke,\n"
			"\t\tGlistens in the web.\n"
			"\t\t"
		);

		src =
			"A haiku:\n"
			"{{quote|indent_str(\"--\")}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"A haiku:\n"
			"--A single raindrop,\n"
			"--Mixed with brine and oily smoke,\n"
			"--Glistens in the web.\n"
			"--"
		);

		src =
			"A haiku:\n"
			"{{indent_str(quote, \"> \")}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"A haiku:\n"
			"> A single raindrop,\n"
			"> Mixed with brine and oily smoke,\n"
			"> Glistens in the web.\n"
			"> "
		);
	}

	TEST(templ_gen, abs_filter) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"value", -42},
		}.dict().value();

		auto src =
			"abs of value is {{value|abs}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"abs of value is 42"
		);

		src =
			"abs of expression is {{-10-2|abs}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"abs of expression is 8"
		);
	}

	TEST(templ_gen, capitilize) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"first_name", "hezekiah"},
			{"last_name", "dombach"},
		}.dict().value();

		auto src =
			"My name is {{first_name|capitilize}} {{last_name|capitilize}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"My name is Hezekiah Dombach"
		);
	}

	TEST(templ_gen, center) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"title_str", "title"}
		}.dict().value();

		auto src =
			"Title format is:\n"
			"{{title_str|center}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Title format is:\n"
			"                                     title"
		);

		src =
			"Title format is:\n"
			"{{title_str|center(12)}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Title format is:\n"
			"   title"
		);

		src =
			"Title format is:\n"
			"{{title_str|capitilize|center(12)}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Title format is:\n"
			"   Title"
		);
	}

	TEST(templ_gen, first) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplObj{
			{"my_list", {"apple", "bannanna", "nuclear rod"}},
		}.dict().value();

		auto src =
			"First element of list is {{my_list|first}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"First element of list is apple"
		);

		args = TemplObj{
			{"my_list", TemplList()}
		}.dict().value();

		EXPECT_KERROR(gen->codegen(src, args), KError::CODEGEN);
	}

	TEST(templ_gen, indent) {
		auto gen = TemplGen::create();
		EXPECT(gen);

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"<quote>\n"
			"> The day has sunk\n"
			"> Into the depths of time untold.\n"
			">  \n"
			"> The dawn has shattered\n"
			"> Leaving fragments strewn about.\n"
			"</quote>\n"
		);
	}

	TEST(templ_gen, macro) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplDict();

		auto src =
			"{\% macro hello() %}\n"
			"Hello World"
			"{\% endmacro %}\n"
			"{{hello()}}\n"
			"{{hello()}}\n";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"Hello World\n"
			"Hello World\n"
		);
	}

	TEST(templ_gen, macro_w_args) {
		auto gen = TemplGen::create();
		EXPECT(gen);

		auto args = TemplDict();

		auto src =
			"{\% macro hello(name, id) %}\n"
			"Hello {{name}}.\n"
			"Your user id is {{id}}.\n"
			"{\% endmacro %}\n"
			"{{hello(\"Alex\", 423)}}"
			"";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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
			gen->codegen(src, args),
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
			gen->codegen(src, args),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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
			gen->codegen(src, args),
			KError::CODEGEN
		);
	}

	TEST(templ_gen, for_loop_var) {
		auto gen = TemplGen::create();

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

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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

	TEST(templ_gen, macro_import) {
		auto gen = TemplGen::create();

		auto args = TemplDict{};

		auto src =
			"{\% include \"assets/tests/common_include.cg\" %}"
			"{{say_hello()}}\n"
			"{{wrap(\"reee\")}}\n";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
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
