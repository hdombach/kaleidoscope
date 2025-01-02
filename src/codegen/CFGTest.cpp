#include "CFG.hpp"
#include "tests/Test.hpp"

namespace cg {
	TEST(cfg, literal) {
		auto literal_cfg = Cfg::literal("simpler cfg");
		EXPECT_EQ(literal_cfg.str(), "\"simpler cfg\"");

		Cfg literal_cfg2 = "another simpler cfg"_cfg;
		EXPECT_EQ(literal_cfg2.str(), "\"another simpler cfg\"");
	}

	TEST(cfg, ref) {
		auto anon_value = "anon"_cfg;

		auto named_value = "named"_cfg;
		named_value.set_name("named_ref");

		auto combined = anon_value + named_value;
		EXPECT_EQ(combined.str(), "<ref> + named_ref");
	}

	TEST(cfg, concat_str) {
		auto first = "f"_cfg + "ir"_cfg + "st"_cfg;
		EXPECT_EQ(first.str(), "\"first\"");

		auto second = Cfg() + Cfg::literal("second") + Cfg();
		EXPECT_EQ(second.str(), "\"second\"");

		auto first_sec = first.dup() + second.dup();
		EXPECT_EQ(first_sec.str(), "\"firstsecond\"");

		auto first_third = Cfg::seq(first.dup(), "third"_cfg);
		EXPECT_EQ(first_third.str(), "\"firstthird\"");
	}

	TEST(cfg, concat) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto third = "third"_cfg;
		third.set_name("third_ref");

		auto first_sec = first + second;
		EXPECT_EQ(first_sec.str(), "first_ref + second_ref");

		auto first_sec2 = first + second.dup();
		EXPECT_EQ(first_sec2.str(), "first_ref + \"second\"");

		auto first_sec3 = first.dup() + second;
		EXPECT_EQ(first_sec3.str(), "\"first\" + second_ref");

		auto first_sec_third = first + second + third;
		EXPECT_EQ(first_sec_third.str(), "first_ref + second_ref + third_ref");

		auto first_sec_third2 = first + second.dup() + third;
		EXPECT_EQ(first_sec_third2.str(), "first_ref + \"second\" + third_ref");

		auto first_sec_third3 = first.dup() + "_"_cfg + "_"_cfg + third;
		EXPECT_EQ(first_sec_third3.str(), "\"first__\" + third_ref");

	}

	TEST(cfg, alt) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto third = "third"_cfg;
		third.set_name("third_ref");

		auto first_sec = first | second;
		EXPECT_EQ(first_sec.str(), "first_ref | second_ref");

		auto first_sec2 = first.dup() | second;
		EXPECT_EQ(first_sec2.str(), "\"first\" | second_ref");

		auto first_sec3 = first | second.dup();
		EXPECT_EQ(first_sec3.str(), "first_ref | \"second\"");

		auto first_sec4 = first.dup() | second.dup();
		EXPECT_EQ(first_sec4.str(), "\"first\" | \"second\"");

		auto first_sec_third = first | second | third;
		EXPECT_EQ(first_sec_third.str(), "first_ref | second_ref | third_ref");

		auto first_sec_third2 = (first | second) | third;
		EXPECT_EQ(first_sec_third2.str(), "first_ref | second_ref | third_ref");

		auto first_sec_third3 = first | (second | third);
		EXPECT_EQ(first_sec_third3.str(), "first_ref | second_ref | third_ref");

		auto first_sec_third4 = (first | second.dup()) | third;
		EXPECT_EQ(first_sec_third4.str(), "first_ref | \"second\" | third_ref");
	}

	TEST(cfg, alt_concat) {
		auto first = "first"_cfg;
		first.set_name("first_ref");
		auto second = "second"_cfg;
		second.set_name("second_ref");
		auto third = "third"_cfg;
		third.set_name("third_ref");
		auto fourth = "fourth"_cfg;
		fourth.set_name("fourth_ref");

		auto test1 = first + second | third;
		EXPECT_EQ(test1.str(), "first_ref + second_ref | third_ref");

		auto test2 = (first + second) | third;
		EXPECT_EQ(test2.str(), "first_ref + second_ref | third_ref");

		auto test3 = first + (second | third);
		EXPECT_EQ(test3.str(), "first_ref + (second_ref | third_ref)");

		auto test4 = first.dup() + (second.dup() | third);
		EXPECT_EQ(test4.str(), "\"first\" + (\"second\" | third_ref)");

		auto test5 = first.dup() + second.dup() | third;
		EXPECT_EQ(test5.str(), "\"firstsecond\" | third_ref");

		auto test_long = (first + second) | (third + fourth);
		EXPECT_EQ(test_long.str(), "first_ref + second_ref | third_ref + fourth_ref");

		auto test_long2 = first + (second | third) + fourth;
		EXPECT_EQ(test_long2.str(), "first_ref + (second_ref | third_ref) + fourth_ref");

		Cfg recurse_test;
		recurse_test = "end"_cfg | "e"_cfg + recurse_test;
		recurse_test.set_name("recurse_test");

		EXPECT_EQ(recurse_test.str(), "\"end\" | \"e\" + recurse_test");
	}

	TEST(cfg, closures) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto single_closure = Cfg::cls(first);
		EXPECT_EQ(single_closure.str(), "[first_ref]");

		auto alt_closure = Cfg::cls(first | second);
		EXPECT_EQ(alt_closure.str(), "[first_ref | second_ref]");

		auto alt_closure2 = Cfg::cls(first.dup() | second.dup());
		EXPECT_EQ(alt_closure2.str(), "[\"first\" | \"second\"]");

		auto seq_closure = Cfg::cls(first + second);
		EXPECT_EQ(seq_closure.str(), "[first_ref + second_ref]");

		auto seq_closure_combined = Cfg::cls(first.dup() + second.dup());
		EXPECT_EQ(seq_closure_combined.str(), "[\"firstsecond\"]");
	}

	TEST(cfg, optional) {
		auto first = "first"_cfg;
		first.set_name("first_ref");

		auto second = "second"_cfg;
		second.set_name("second_ref");

		auto third = "third"_cfg;
		third.set_name("third_ref");

		auto single = Cfg::opt(first);
		EXPECT_EQ(single.str(), "(first_ref)?");

		auto single_literal = Cfg::opt(first.dup());
		EXPECT_EQ(single_literal.str(), "(\"first\")?");

		auto concat_opt = Cfg::opt(first + second) + third;
		EXPECT_EQ(concat_opt.str(), "(first_ref + second_ref)? + third_ref");

		auto combine_opt = Cfg::opt(first.dup() + second.dup()) + third;
		EXPECT_EQ(combine_opt.str(), "(\"firstsecond\")? + third_ref");

		auto alt_opt = Cfg::opt(first | second) | third;
		EXPECT_EQ(alt_opt.str(), "(first_ref | second_ref)? | third_ref");

		auto mixed_opt = first | Cfg::opt(second + third);
		EXPECT_EQ(mixed_opt.str(), "first_ref | (second_ref + third_ref)?");

		auto mixed_opt2 = Cfg::opt(second | third) + first;
		EXPECT_EQ(mixed_opt2.str(), "(second_ref | third_ref)? + first_ref");

		auto opt_nested = Cfg::opt(Cfg::opt(first.dup()));
		EXPECT_EQ(opt_nested.str(), "((\"first\")?)?");

	}
}
