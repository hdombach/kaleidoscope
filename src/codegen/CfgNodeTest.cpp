#include "CfgNode.hpp"
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"

namespace cg {
	TEST(cfg_node, literal) {
		CfgContext c;

		c.prim("literal") = c.s("simple_cfg");
		c.prim("literal2") = c.s("another_simpl_cfg");

		c.prep();

		EXPECT_EQ(c.get("literal")->str(), "literal -> \"simple_cfg\"");
		EXPECT_EQ(c.get("literal2")->str(), "literal2 -> \"another_simpl_cfg\"");
	}

	TEST(cfg_node, ref) {
		auto c = CfgContext();

		c.prim("first_ref") = c.s("first");
		c.prim("second_ref") = c.s("second");
		c.prim("combined") = c["first_ref"] + c["second_ref"];

		c.prep();

		EXPECT_EQ(c.get("combined")->str(), "combined -> <first_ref> <second_ref>");
	}

	TEST(cfg_node, concat_str) {
		auto c = CfgContext();

		c.prim("first") = c.s("f") + c.s("ir") + c.s("st");
		c.prim("second") = c.s("second");
		c.prim("first_sec") = c["first"] + c["second"];
		c.prim("first_third") = c["first"] + c["third"];

		EXPECT_EQ(c.get("first")->str(), "first -> \"f\" \"ir\" \"st\"");
		EXPECT_EQ(c.get("second")->str(), "second -> \"second\"");
		EXPECT_EQ(c.get("first_sec")->str(), "first_sec -> <first> <second>");
	}

	TEST(cfg_node, concat) {
		auto c = CfgContext();

		c.prim("first_ref") = c.s("first");
		c.prim("second_ref") = c.s("second");
		c.prim("third_ref") = c.s("third");

		c.prim("first_sec") = c["first_ref"] + c["second_ref"];
		c.prim("first_sec_third") = c["first_ref"] + c["second_ref"] + c["third_ref"];
		c.prim("first_sec_third2") = c["first_ref"]
			+ c.s("_") + c["second_ref"] + c.s("_")
			+ c["third_ref"];

		c.prep();

		EXPECT_EQ(c.get("first_sec")->str(), "first_sec -> <first_ref> <second_ref>");
		EXPECT_EQ(c.get("first_sec_third")->str(), "first_sec_third -> <first_ref> <second_ref> <third_ref>");
		EXPECT_EQ(c.get("first_sec_third2")->str(), "first_sec_third2 -> <first_ref> \"_\" <second_ref> \"_\" <third_ref>");
	}

	TEST(cfg_node, alt) {
		auto c = CfgContext();

		c.prim("first_ref") = c.s("first");
		c.prim("second_ref") = c.s("second");
		c.prim("third_ref") = c.s("third");

		c.prim("first_sec") = c["first_ref"] | c["second_ref"];
		c.prim("first_sec2") = c["first_ref"] | c.s("second");
		c.prim("first_sec_third") = c["first_ref"] | c["second_ref"] | c["third_ref"];
		c.prim("first_sec_third2") = c["first_ref"] | c.s("second_ref") | c["third_ref"];

		c.prep();

		EXPECT_EQ(c.get("first_sec")->str(), "first_sec -> <first_ref> | <second_ref>");
		EXPECT_EQ(c.get("first_sec2")->str(), "first_sec2 -> <first_ref> | \"second\"");
		EXPECT_EQ(c.get("first_sec_third")->str(), "first_sec_third -> <first_ref> | <second_ref> | <third_ref>");
		EXPECT_EQ(c.get("first_sec_third2")->str(),  "first_sec_third2 -> <first_ref> | \"second_ref\" | <third_ref>");
	}

	TEST(cfg_node, alt_concat) {
		auto c = CfgContext();

		c.prim("first_ref") = c.s("first");
		c.prim("second_ref") = c.s("second");
		c.prim("third_ref") = c.s("third");

		c.prim("test1")
			= c["first_ref"] + c.s("ree")
			| c["first_ref"] + c["second_ref"];

		c.prim("test2")
			= c.s("yeah boi")
			| c["first_ref"] + c.s("first") + c["second_ref"]
			| c.s("last");

		c.prim("test3")
			= c["first_ref"]
			| c["second_ref"]
			| c.s("first") + c.s("second");

		c.prep();

		EXPECT_EQ(c.get("test1")->str(), "test1 -> <first_ref> \"ree\" | <first_ref> <second_ref>");
		EXPECT_EQ(c.get("test2")->str(), "test2 -> \"yeah boi\" | <first_ref> \"first\" <second_ref> | \"last\"");
		EXPECT_EQ(c.get("test3")->str(), "test3 -> <first_ref> | <second_ref> | \"first\" \"second\"");
	}
}
