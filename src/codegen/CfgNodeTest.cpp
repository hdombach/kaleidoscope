#include "CfgNode.hpp"
#include "codegen/CfgContext.hpp"
#include "TemplTokenizer.hpp"
#include "tests/Test.hpp"

namespace cg {
	TEST(cfg_node, literal) {
		CfgContext c;
		using T = TemplTokenType;

		c.root("root") = c["literal"] + T::Eof;

		c.prim("literal") = int(T::StrConst);
		c.prim("literal2") = int(T::IntConst);

		EXPECT(c.prep());

		EXPECT_EQ(c.get("literal")->str(TEMPL_TOK_CONFIG), "literal -> StrConstant");
		EXPECT_EQ(c.get("literal2")->str(TEMPL_TOK_CONFIG), "literal2 -> IntConstant");
	}

	TEST(cfg_node, concat) {
		auto c = CfgContext();
		using T = TemplTokenType;

		c.root("root") = c["if_ref"] + T::Eof;

		c.prim("if_ref") = int(T::If);
		c.prim("condition") = T::ParanOpen + T::StrConst + T::ParanClose;

		c.prim("if_else") = c["if_ref"] + c["condition"] + T::Else;

		c.prim("test") = T::CommentB + T::CommentE + c["condition"];
		c.prim("test2") = c["test"] + T::ExpB + T::ExpE;

		EXPECT(c.prep());

		EXPECT_EQ(c.get("if_ref")->str(TEMPL_TOK_CONFIG), "if_ref -> If");
		EXPECT_EQ(c.get("condition")->str(TEMPL_TOK_CONFIG), "condition -> ParanOpen StrConstant ParanClose");
		EXPECT_EQ(c.get("if_else")->str(TEMPL_TOK_CONFIG), "if_else -> <if_ref> <condition> Else");
		EXPECT_EQ(c.get("test")->str(TEMPL_TOK_CONFIG), "test -> CommentB CommentE <condition>");
		EXPECT_EQ(c.get("test2")->str(TEMPL_TOK_CONFIG), "test2 -> <test> ExpB ExpE");
	}

	TEST(cfg_node, alt) {
		auto c = CfgContext();
		using T = TemplTokenType;

		c.root("root") = c["tok_ref"] + T::Eof;
		c.prim("tok_ref") = T::ParanOpen | c["tok_ref"];
		c.prim("ref_tok") = c["ref_tok"] | T::ParanClose;
		c.prim("tok_tok") = T::IntConst | T::Ident;
		c.prim("ref_ref") = c["ref_ref"] | c["tok_ref"];

		c.prim("concat_tok") = T::ParanOpen + T::ParanClose | T::IntConst;
		c.prim("concat_ref") = T::ParanOpen + T::ParanClose | c["concat_ref"];

		c.prim("tok_concat") = T::IntConst | T::ParanOpen + T::ParanClose;
		c.prim("ref_concat") = c["ref_concat"] | T::ParanOpen + T::ParanClose;

		EXPECT(c.prep());

		EXPECT_EQ(c.get("root")->str(TEMPL_TOK_CONFIG), "root -> <tok_ref> EOF");
		EXPECT_EQ(c.get("tok_ref")->str(TEMPL_TOK_CONFIG), "tok_ref -> ParanOpen | <tok_ref>");
		EXPECT_EQ(c.get("ref_tok")->str(TEMPL_TOK_CONFIG), "ref_tok -> <ref_tok> | ParanClose");
		EXPECT_EQ(c.get("tok_tok")->str(TEMPL_TOK_CONFIG), "tok_tok -> IntConstant | Identifier");
		EXPECT_EQ(c.get("ref_ref")->str(TEMPL_TOK_CONFIG), "ref_ref -> <ref_ref> | <tok_ref>");
		EXPECT_EQ(c.get("concat_tok")->str(TEMPL_TOK_CONFIG), "concat_tok -> ParanOpen ParanClose | IntConstant");
		EXPECT_EQ(c.get("concat_ref")->str(TEMPL_TOK_CONFIG), "concat_ref -> ParanOpen ParanClose | <concat_ref>");
		EXPECT_EQ(c.get("tok_concat")->str(TEMPL_TOK_CONFIG), "tok_concat -> IntConstant | ParanOpen ParanClose");
		EXPECT_EQ(c.get("ref_concat")->str(TEMPL_TOK_CONFIG), "ref_concat -> <ref_concat> | ParanOpen ParanClose");
	}
}
