#include "CfgNode.hpp"
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"

namespace cg {
	TEST(cfg_node, literal) {
		CfgContext c;
		using T = CfgLeaf::TType;

		c.root("literal") = T::StrConst;
		c.prim("literal2") = T::IntConst;

		EXPECT(c.prep());

		EXPECT_EQ(c.get("literal")->str(), "literal -> StrConstant");
		EXPECT_EQ(c.get("literal2")->str(), "literal2 -> IntConstant");
	}

	TEST(cfg_node, concat) {
		auto c = CfgContext();
		using T = CfgLeaf::TType;

		c.root("if_ref") = T::If;
		c.prim("condition") = T::ParanOpen + T::StrConst + T::ParanClose;

		c.prim("if_else") = c["if_ref"] + c["condition"] + T::Else;

		c.prim("test") = T::CommentB + T::CommentE + c["condition"];
		c.prim("test2") = c["test"] + T::ExpB + T::ExpE;

		EXPECT(c.prep());

		EXPECT_EQ(c.get("if_ref")->str(), "if_ref -> If");
		EXPECT_EQ(c.get("condition")->str(), "condition -> ParanOpen StrConstant ParanClose");
		EXPECT_EQ(c.get("if_else")->str(), "if_else -> <if_ref> <condition> Else");
		EXPECT_EQ(c.get("test")->str(), "test -> CommentB CommentE <condition>");
		EXPECT_EQ(c.get("test2")->str(), "test2 -> <test> ExpB ExpE");
	}

	TEST(cfg_node, alt) {
		auto c = CfgContext();
		using T = CfgLeaf::TType;

		c.root("tok_ref") = T::ParanOpen | c["tok_ref"];
		c.prim("ref_tok") = c["ref_tok"] | T::ParanClose;
		c.prim("tok_tok") = T::IntConst | T::Ident;
		c.prim("ref_ref") = c["ref_ref"] | c["tok_ref"];

		c.prim("concat_tok") = T::ParanOpen + T::ParanClose | T::IntConst;
		c.prim("concat_ref") = T::ParanOpen + T::ParanClose | c["concat_ref"];

		c.prim("tok_concat") = T::IntConst | T::ParanOpen + T::ParanClose;
		c.prim("ref_concat") = c["ref_concat"] | T::ParanOpen + T::ParanClose;

		EXPECT(c.prep());

		EXPECT_EQ(c.get("tok_ref")->str(), "tok_ref -> ParanOpen | <tok_ref>");
		EXPECT_EQ(c.get("ref_tok")->str(), "ref_tok -> <ref_tok> | ParanClose");
		EXPECT_EQ(c.get("tok_tok")->str(), "tok_tok -> IntConstant | Identifier");
		EXPECT_EQ(c.get("ref_ref")->str(), "ref_ref -> <ref_ref> | <tok_ref>");
		EXPECT_EQ(c.get("concat_tok")->str(), "concat_tok -> ParanOpen ParanClose | IntConstant");
		EXPECT_EQ(c.get("concat_ref")->str(), "concat_ref -> ParanOpen ParanClose | <concat_ref>");
		EXPECT_EQ(c.get("tok_concat")->str(), "tok_concat -> IntConstant | ParanOpen ParanClose");
		EXPECT_EQ(c.get("ref_concat")->str(), "ref_concat -> <ref_concat> | ParanOpen ParanClose");
	}
}
