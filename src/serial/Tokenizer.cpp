#include "Tokenizer.hpp"

using namespace cg;

namespace serial {
	const Token::Config TOK_CONFIG = {
		{
			"Unmatched",
			"EOF",
			"Comment",
			"Whitespace",
			"Less",
			"Greater",
			"OpenCurly",
			"CloseCurly",
			"Period",
			"Equal",
			"Semicolon",
			"IntConst",
			"StrConst",
			"Divide",
			"Struct",
			"Enum",
			"Bitfield",
			"Float",
			"Double",
			"Boolean",
			"U8",
			"U16",
			"U32",
			"U64",
			"I8",
			"I16",
			"I32",
			"I64",
			"String",
			"Array",
			"Optional",
			"UIDList",
			"Version",
			"Include",
			"Identifier",
		},
		{
			std::regex(""), // Unmatched
			std::regex(""), // Eof,
			std::regex("\\/\\*(\\*(?!\\/)|[^*])*\\*\\/"), // Comment
			std::regex("([ \\t\\n])+"), // Whitespace
			std::regex("<"), // Less
			std::regex(">"), // Greater
			std::regex("\\{"), // OpenCurly
			std::regex("\\}"), // CloseCurly
			std::regex("\\."), // Period
			std::regex("="), // Equal
			std::regex(";"), // Semicolon
			std::regex("\\d+"), // IntConst
			std::regex("\"([^\\\\\"]|(\\\\.))*\""), // StrConst
			std::regex("\\/"),
			std::regex("struct"),
			std::regex("enum"),
			std::regex("bitfield"),
			std::regex("float"),
			std::regex("double"),
			std::regex("boolean"),
			std::regex("u8"),
			std::regex("u16"),
			std::regex("u32"),
			std::regex("u64"),
			std::regex("i8"),
			std::regex("i16"),
			std::regex("i32"),
			std::regex("i64"),
			std::regex("string"),
			std::regex("array"),
			std::regex("optional"),
			std::regex("uidlist"),
			std::regex("version"),
			std::regex("include"),
			std::regex("[a-zA-Z_][a-zA-Z0-9_]*"), // Identifier
		}
	};

	CfgRule operator + (TokenType const &lhs, TokenType const &rhs) {
		return CfgLeaf(int(lhs)) + CfgLeaf(int(rhs));
	}

	CfgRule operator + (CfgLeaf const &lhs, TokenType const &rhs) {
		return lhs + CfgLeaf(int(rhs));
	}

	CfgRule operator + (TokenType const &lhs, CfgLeaf const &rhs) {
		return CfgLeaf(int(lhs)) + rhs;
	}

	CfgRule operator + (CfgRule const &lhs, TokenType const &rhs) {
		return lhs + CfgLeaf(int(rhs));
	}

	CfgRuleSet operator | (TokenType const &lhs, TokenType const &rhs) {
		return CfgLeaf(int(lhs)) | CfgLeaf(int(rhs));
	}

	CfgRuleSet operator | (TokenType const &lhs, CfgLeaf const &rhs) {
		return CfgLeaf(int(lhs)) | rhs;
	}

	CfgRuleSet operator | (TokenType const &lhs, CfgRule const &rhs) {
		return CfgLeaf(int(lhs)) | rhs;
	}

	CfgRuleSet operator | (CfgRule const &lhs, TokenType const &rhs) {
		return lhs | CfgLeaf(int(rhs));
	}

	CfgRuleSet operator | (CfgLeaf const &lhs, TokenType const &rhs) {
		return lhs | CfgLeaf(int(rhs));
	}

	CfgRuleSet operator | (CfgRuleSet const &lhs, TokenType const &rhs) {
		return lhs | CfgLeaf(int(rhs));
	}
}
