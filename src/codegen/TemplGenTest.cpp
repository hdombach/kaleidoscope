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
			{"combine_str", mk_templfunc(std::function(combined))}
		}.dict().value();

		auto src = "User id is {{combine_str(name, id)}}";

		EXPECT_EQ(
			gen->codegen(src, args).value(),
			"User id is Bob59"
		);
	}
}
