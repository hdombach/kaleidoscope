#include "TemplTokenizer.hpp"
#include "codegen/CfgNode.hpp"

namespace cg {
	const Token::Config TEMPL_TOK_CONFIG = {
		{
			"Unmatched",
			"EOF",
			"Padding",
			"Newline",
			"CommentB",
			"CommentE",
			"ExpB",
			"ExpE",
			"StmtB",
			"StmtE",
			"If",
			"Elif",
			"Else",
			"Endif",
			"For",
			"In",
			"EndFor",
			"Macro",
			"EndMacro",
			"Include",
			"Identifier",
			"StrConstant",
			"IntConstant",
			"ParanOpen",
			"ParanClose",
			"Plus",
			"Minus",
			"Multiply",
			"Divide",
			"Percentage",
			"Period",
			"Comma",
			"GreaterEqual",
			"Greater",
			"LesserEqual",
			"Lesser",
			"Equal",
			"NotEqual",
			"Exclamation",
			"LogicalAnd",
			"LogicalOr",
			"Bar",
			"Assignment",
		},
		{
			std::regex(""), // Unmatched
			std::regex(""), // Eof,
			std::regex("([ \\t])+"), // Pad
			std::regex("\\n"), // Newline
			std::regex("\\{#(-|\\+)?"), // CommentB
			std::regex("(-|\\+)?#\\}"), // CommentE
			std::regex("\\{\\{(-|\\+)?"), // ExpB
			std::regex("(-|\\+)?\\}\\}"), // ExpE
			std::regex("\\{%(-|\\+)?"), // StmtB
			std::regex("(-|\\+)?%\\}"), // StmtE
			std::regex("if(?![\\w])"),
			std::regex("elif(?![\\w])"),
			std::regex("else(?![\\w])"),
			std::regex("endif(?![\\w])"),
			std::regex("for(?![\\w])"),
			std::regex("in(?![\\w])"),
			std::regex("endfor(?![\\w])"),
			std::regex("macro(?![\\w])"),
			std::regex("endmacro(?![\\w])"),
			std::regex("include(?![\\w])"),
			std::regex("[_a-zA-Z][\\w]*"), // Ident
			std::regex("\"([^\\\\\"]|(\\\\.))*\""), // StrConst
			std::regex("\\d+"), // IntConst
			std::regex("\\("), // ParanOpen
			std::regex("\\)"), // ParanClose
			std::regex("\\+"), // Plus
			std::regex("\\-"), // Minus
			std::regex("\\*"), // Mult
			std::regex("\\/"), // Div
			std::regex("%"), // Perc
			std::regex("\\."), // Period
			std::regex(","), // Comma
			std::regex(">="), // GreatEq
			std::regex(">"), // Great
			std::regex("<="), // LessEq
			std::regex("<"), // Less
			std::regex("=="), // Equal
			std::regex("!="), // NotEqual
			std::regex("!"), // Excl
			std::regex("&&"), // LAnd
			std::regex("\\|\\|"), // LOr
			std::regex("\\|"), // Bar
			std::regex("="), // Assignment
		},
		true,
	};

	std::vector<Token> simplify_templ_tokens(std::vector<Token> const &tokens) {
		auto result = std::vector<Token>();
		using T = TemplTokenType;

		bool in_statement = false;
		auto cur_token = Token();
		int skipped_pad = 0;
		for (auto i = 0; i < tokens.size(); i++) {
			auto &t = tokens[i];

			//Handle remove padding before and after
			if (t.type() == int(T::ExpB) || t.type() == int(T::StmtB) || t.type() == int(T::CommentB)) {
				Token const *close_tag = &t;
				while (close_tag->type() != int(T::StmtE) && close_tag->type() != int(T::CommentE) && close_tag->type() != int(T::ExpE)) {
					log_assert(close_tag->type() != int(TemplTokenType::Eof), "There must be an ending tag before Eof");
					close_tag++;
				}

				// We want to keep padding while expressions and remove padding around
				// statmenets and comments by default
				bool is_sole_line = t.type() == int(T::StmtB) || t.type() == int(T::CommentB);
				if (i == 0) {
				} else if (i >= 1 && tokens[i-1].type() == int(T::Newline)) {
				} else if (i >=2 && tokens[i-1].type() == int(T::Pad) && tokens[i-2].type() == int(T::Newline)) {
				} else {
					is_sole_line = false;
				}

				if (close_tag[1].type() == int(T::Newline)) {
				} else if (close_tag[1].type() == int(T::Pad) && close_tag[2].type() == int(T::Newline)) {
				} else {
					is_sole_line = false;
				}

				if (t.content()[2] == '-') {
					while (!result.empty() && (result.back().type() == int(T::Newline) || result.back().type() == int(T::Pad))) {
						result.pop_back();
					}
				}
				if (t.content()[2] == '\0' && is_sole_line) {
					// Just skip previous padding
					if (!result.empty() && result.back().type() == int(T::Pad)) result.pop_back();
				}

				skipped_pad = 0;
				if (close_tag->content()[0] == '-') {
					close_tag++;
					while (close_tag->type() == int(T::Newline) || close_tag->type() == int(T::Pad)) {
						skipped_pad++;
						close_tag++;
					}
				} else if (close_tag->content()[0] != '+' && is_sole_line) {
					// Skip padding and one newline (default behavior)
					close_tag++;
					while ((close_tag->type() == int(T::Newline)) || close_tag->type() == int(T::Pad)) {
						skipped_pad++;
						// Break after we find the first newline
						if (close_tag->type() == int(T::Newline)) break;
						close_tag++;
					}
				}
			}

			// Handle making everything outside statements unmatched
			if (in_statement) {
				result.push_back(t);
				if (
					t.type() == int(T::ExpE)
					|| t.type() == int(T::StmtE)
				) {
					in_statement = false;
				}
			} else {
				if (
					t.type() == int(T::Pad)
					|| t.type() == int(T::Newline)
					|| t.type() == int(T::Eof)
					|| t.type() == int(T::CommentB)
					|| t.type() == int(T::CommentE)
				) {
					if (cur_token.exists())
						result.push_back(cur_token);
					cur_token = Token();
					if (skipped_pad > 0 && (t.type() == int(T::Pad) || t.type() == int(T::Newline))) {
						skipped_pad--;
					} else {
						result.push_back(t);
					}
				} else if (
					t.type() == int(T::ExpB)
					|| t.type() == int(T::StmtB)
				) {
					in_statement = true;
					if (cur_token.exists())
						result.push_back(cur_token);
					cur_token = Token();
					result.push_back(t);
				} else {
					cur_token += t;
				}
			}
		}

		return result;
	}
}

namespace cg {

	/******** CfgRule overwrites ********/
	CfgRule operator + (
		TemplTokenType const &lhs,
		TemplTokenType const &rhs
	) {
		return CfgLeaf(int(lhs)) + CfgLeaf(int(rhs));
	}

	CfgRule operator + (
		CfgLeaf const &lhs,
		TemplTokenType const &rhs
	) {
		return lhs + CfgLeaf(int(rhs));
	}

	CfgRule operator + (
		TemplTokenType const &lhs,
		CfgLeaf const &rhs
	) {
		return CfgLeaf(int(lhs)) + rhs;
	}

	CfgRule operator + (
		CfgRule const &lhs,
		TemplTokenType const &rhs
	) {
		return lhs + CfgLeaf(int(rhs));
	}

	/******** CfgRuleSet overwrites ********/
	CfgRuleSet operator | (
		TemplTokenType const &lhs,
		TemplTokenType const &rhs
	) {
		return CfgLeaf(int(lhs)) | CfgLeaf(int(rhs));
	}

	CfgRuleSet operator | (
		TemplTokenType const &lhs,
		CfgLeaf const &rhs
	) {
		return CfgLeaf(int(lhs)) | rhs;
	}

	CfgRuleSet operator | (
		TemplTokenType const &lhs,
		CfgRule const &rhs
	) {
		return CfgLeaf(int(lhs)) | rhs;
	}

	CfgRuleSet operator | (
		CfgRule const &lhs,
		TemplTokenType const &rhs
	) {
		return lhs | CfgLeaf(int(rhs));
	}

	CfgRuleSet operator | (
		CfgLeaf const &lhs,
		TemplTokenType const &rhs
	) {
		return lhs | CfgLeaf(int(rhs));
	}

	CfgRuleSet operator | (
		CfgRuleSet const &lhs,
		TemplTokenType const &rhs
	) {
		return lhs | CfgLeaf(int(rhs));
	}
}

