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
			"My name is Hezekiah Dombach!"
		);
	}
}
