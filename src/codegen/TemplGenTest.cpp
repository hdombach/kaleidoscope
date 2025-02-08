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

		auto args = TemplObj::Dict{
			{"name", TemplObj("Hezekiah Dombach")}
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

		auto args = TemplObj::Dict{
			{"shopping_list", TemplObj::List{
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

		auto args = TemplObj::Dict{
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

		auto args = TemplObj::Dict{
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

		auto args = TemplObj::Dict{
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

		auto args = TemplObj::Dict{
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

		auto args = TemplObj::Dict{
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

		auto args = TemplObj::Dict{
			{
				"person", TemplObj::Dict{
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
}
