#pragma once

#include "Tokenizer.hpp"

namespace cg {
	enum class TemplTokenType: int {
		Unmatched = int(Token::Type::Unmatched),
		Eof = int(Token::Type::Eof),
		Pad,
		Newline,
		CommentB,
		CommentE,
		ExpB,
		ExpE,
		StmtB,
		StmtE,
		If,
		Elif,
		Else,
		Endif,
		For,
		In,
		EndFor,
		Macro,
		Endmacro,
		Include,
		Ident,
		StrConst,
		IntConst,
		ParanOpen,
		ParanClose,
		Plus,
		Minus,
		Mult,
		Div,
		Perc,
		Period,
		Comma,
		GreatEq,
		Great,
		LessEq,
		Less,
		Equal,
		NotEqual,
		Excl,
		LAnd,
		LOr,
		Bar,
		Assignment,
	};

	extern const Token::Config TEMPL_TOK_CONFIG;

	/**
	 * Combines every token not in a statement into an unmatched token
	 * Removes padding before and after statements and comments
	 */
	std::vector<Token> simplify_templ_tokens(std::vector<Token> const &tokens);

	class CfgLeaf;
	class CfgRule;
	class CfgRuleSet;

	/******** CfgRule overwrites ********/

	CfgRule operator + (TemplTokenType const &lhs, TemplTokenType const &rhs);

	CfgRule operator + (CfgLeaf const &lhs, TemplTokenType const &rhs);

	CfgRule operator + (TemplTokenType const &lhs, CfgLeaf const &rhs);

	CfgRule operator + (CfgRule const &lhs, TemplTokenType const &rhs);

	/******** CfgRuleSet overwrites ********/

	CfgRuleSet operator | (TemplTokenType const &lhs, TemplTokenType const &rhs);

	CfgRuleSet operator | (TemplTokenType const &lhs, CfgLeaf const &rhs);

	CfgRuleSet operator | (TemplTokenType const &lhs, CfgRule const &rhs);

	CfgRuleSet operator | (CfgRule const &lhs, TemplTokenType const &rhs);

	CfgRuleSet operator | (CfgLeaf const &lhs, TemplTokenType const &rhs);

	CfgRuleSet operator | (CfgRuleSet const &lhs, TemplTokenType const &rhs);
}
