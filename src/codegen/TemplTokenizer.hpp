#pragma once

#include "Tokenizer.hpp"

namespace cg {
	enum class TemplTokenType: int {
		Unmatched = int(Token::Type::Unmatched),
		Eof = int(Token::Type::Eof),
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
		Mult,
		Div,
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
		ExpE,
		StmtE,
		Plus,
		Minus,
		Perc,
		CommentE,
		Pad,
		Newline,
		ExpB,
		StmtB,
		CommentB,
		Raw,
	};

	extern const Token::Config TEMPL_TOK_CONFIG;

	std::vector<Token> tokenize_templ(util::StringRef str);

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
