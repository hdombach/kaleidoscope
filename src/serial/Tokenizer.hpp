#pragma once

#include "codegen/Tokenizer.hpp"
#include "codegen/CfgNode.hpp"

namespace serial {
	enum class TokenType: int {
		Unmatched = int(cg::Token::Type::Unmatched),
		Eof = int(cg::Token::Type::Eof),
		Comment,
		Whitespace,
		Less,
		Greater,
		OpenCurly,
		CloseCurly,
		Period,
		Equal,
		Semicolon,
		IntConst,
		StrConst,
		Divide,
		Struct,
		Enum,
		Bitfield,
		Float,
		Double,
		Boolean,
		U8,
		U16,
		U32,
		U64,
		I8,
		I16,
		I32,
		I64,
		String,
		Array,
		Optional,
		UIDList,
		Version,
		Include,
		Identifier,
	};

	extern const cg::Token::Config TOK_CONFIG;

	/******** CfgRule overwrites ********/

	cg::CfgRule operator + (TokenType const &lhs, TokenType const &rhs);

	cg::CfgRule operator + (cg::CfgLeaf const &lhs, TokenType const &rhs);

	cg::CfgRule operator + (TokenType const &lhs, cg::CfgLeaf const &rhs);

	cg::CfgRule operator + (cg::CfgRule const &lhs, TokenType const &rhs);

	/******** CfgRuleSet overites ********/
	
	cg::CfgRuleSet operator | (TokenType const &lhs, TokenType const &rhs);

	cg::CfgRuleSet operator | (TokenType const &lhs, cg::CfgLeaf const &rhs);

	cg::CfgRuleSet operator | (TokenType const &lhs, cg::CfgRule const &rhs);

	cg::CfgRuleSet operator | (cg::CfgRule const &lhs, TokenType const &rhs);

	cg::CfgRuleSet operator | (cg::CfgLeaf const &lhs, TokenType const &rhs);

	cg::CfgRuleSet operator | (cg::CfgRuleSet const &lhs, TokenType const &rhs);
}

