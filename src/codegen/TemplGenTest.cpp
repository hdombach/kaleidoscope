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
}
