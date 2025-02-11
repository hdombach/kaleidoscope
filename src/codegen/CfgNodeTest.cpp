#include "CfgNode.hpp"
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"

namespace cg {
	TEST(cfg_node, literal) {
		CfgContext c;

		c.prim("literal") = c.lit("simple_cfg");
		c.prim("literal2") = c.lit("another_simpl_cfg");

		c.prep();

		EXPECT_EQ(c.node_str("literal"), "\"simple_cfg\"");
		EXPECT_EQ(c.node_str("literal2"), "\"another_simpl_cfg\"");
	}

	TEST(cfg_node, ref) {
		auto c = CfgContext();

		c.prim("first_ref") = c.lit("first");
		c.prim("second_ref") = c.lit("second");
		c.prim("combined") = c.ref("first_ref") + c.ref("second_ref");

		c.prep();

		EXPECT_EQ(c.node_str("combined"), "<first_ref> + <second_ref>");
	}

	TEST(cfg_node, concat_str) {
		auto c = CfgContext();

		c.prim("first") = c.lit("f") + c.lit("ir") + c.lit("st");
		c.prim("second") = c.lit("second");
		c.prim("first_sec") = c.dup("first") + c.dup("second");
		c.prim("first_third") = c.dup("first") + c.lit("third");

		EXPECT_EQ(c.node_str("first"), "\"first\"");
		EXPECT_EQ(c.node_str("second"), "\"second\"");
		EXPECT_EQ(c.node_str("first_sec"), "\"firstsecond\"");
	}

	TEST(cfg_node, concat) {
		auto c = CfgContext();

		c.prim("first_ref") = c.lit("first");
		c.prim("second_ref") = c.lit("second");
		c.prim("third_ref") = c.lit("third");

		c.prim("first_sec") = c.ref("first_ref") + c.ref("second_ref");
		c.prim("first_sec2") = c.ref("first_ref") + c.dup("second_ref");
		c.prim("first_sec_third") = c.ref("first_ref") + c.ref("second_ref") + c.ref("third_ref");
		c.prim("first_sec_third2") = c.ref("first_ref")
			+ (c.lit("_") + c.dup("second_ref") + c.lit("_"))
			+ c.ref("third_ref");

		c.prep();

		EXPECT_EQ(c.node_str("first_sec"), "<first_ref> + <second_ref>");
		EXPECT_EQ(c.node_str("first_sec2"), "<first_ref> + \"second\"");
		EXPECT_EQ(c.node_str("first_sec_third"), "<first_ref> + <second_ref> + <third_ref>");
		EXPECT_EQ(c.node_str("first_sec_third2"), "<first_ref> + \"_second_\" + <third_ref>");
	}

	TEST(cfg_node, alt) {
		auto c = CfgContext();

		c.prim("first_ref") = c.lit("first");
		c.prim("second_ref") = c.lit("second");
		c.prim("third_ref") = c.lit("third");

		c.prim("first_sec") = c.ref("first_ref") | c.ref("second_ref");
		c.prim("first_sec2") = c.dup("first_ref") | c.ref("second_ref");
		c.prim("first_sec3") = c.ref("first_ref") | c.lit("second");
		c.prim("first_sec4") = c.dup("first_ref") | c.dup("second_ref");
		c.prim("first_sec_third") = (c.ref("first_ref") | c.ref("second_ref")) | c.ref("third_ref");
		c.prim("first_sec_third2") = c.ref("first_ref") | c.ref("second_ref") | c.ref("third_ref");
		c.prim("first_sec_third3") = c.ref("first_ref") | (c.ref("second_ref") | c.ref("third_ref"));
		c.prim("first_sec_third4") = (c.ref("first_ref") | c.lit("second")) | c.ref("third_ref");

		c.prep();

		EXPECT_EQ(c.node_str("first_sec"), "<first_ref> | <second_ref>");
		EXPECT_EQ(c.node_str("first_sec2"), "\"first\" | <second_ref>");
		EXPECT_EQ(c.node_str("first_sec3"), "<first_ref> | \"second\"");
		EXPECT_EQ(c.node_str("first_sec4"), "\"first\" | \"second\"");
		EXPECT_EQ(c.node_str("first_sec_third"), "<first_ref> | <second_ref> | <third_ref>");
		EXPECT_EQ(c.node_str("first_sec_third2"), "<first_ref> | <second_ref> | <third_ref>");
		EXPECT_EQ(c.node_str("first_sec_third3"), "<first_ref> | <second_ref> | <third_ref>");
		EXPECT_EQ(c.node_str("first_sec_third4"),  "<first_ref> | \"second\" | <third_ref>");
	}

	TEST(cfg_node, alt_concat) {
		auto c = CfgContext();

		c.prim("first_ref") = c.lit("first");
		c.prim("second_ref") = c.lit("second");
		c.prim("third_ref") = c.lit("third");
		c.prim("fourth_ref") = c.lit("fourth");

		c.prim("test1") = c.ref("first_ref") + c.ref("second_ref") | c.ref("third_ref");
		c.prim("test2") = (c.ref("first_ref") + c.ref("second_ref")) | c.ref("third_ref");
		c.prim("test3") = c.ref("first_ref") + (c.ref("second_ref") | c.ref("third_ref"));
		c.prim("test4") = c.dup("first_ref") + (c.dup("second_ref") | c.ref("third_ref"));
		c.prim("test5") = c.dup("first_ref") + c.dup("second_ref") | c.ref("third_ref");
		c.prim("test_long") = (c.ref("first_ref") + c.ref("second_ref")) | (c.ref("third_ref") + c.ref("fourth_ref"));
		c.prim("test_long2") = c.ref("first_ref") + (c.ref("second_ref") | c.ref("third_ref")) + c.ref("fourth_ref");
		c.prim("recurse_test") = c.lit("end") | c.lit("e") + c.ref("recurse_test");

		c.prep();

		EXPECT_EQ(c.node_str("test1"), "<first_ref> + <second_ref> | <third_ref>");
		EXPECT_EQ(c.node_str("test2"), "<first_ref> + <second_ref> | <third_ref>");
		EXPECT_EQ(c.node_str("test3"), "<first_ref> + (<second_ref> | <third_ref>)");
		EXPECT_EQ(c.node_str("test4"), "\"first\" + (\"second\" | <third_ref>)");
		EXPECT_EQ(c.node_str("test5"), "\"firstsecond\" | <third_ref>");
		EXPECT_EQ(c.node_str("test_long"), "<first_ref> + <second_ref> | <third_ref> + <fourth_ref>");
		EXPECT_EQ(c.node_str("test_long2"), "<first_ref> + (<second_ref> | <third_ref>) + <fourth_ref>");
		EXPECT_EQ(c.node_str("recurse_test"), "\"end\" | \"e\" + <recurse_test>");
	}
}
