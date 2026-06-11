#include "TemplTokenizer.hpp"
#include "codegen/CfgNode.hpp"
#include "codegen/Error.hpp"
#include "util/log.hpp"

#include <regex>

namespace cg {
	const Token::Config TEMPL_TOK_CONFIG = {
		{
			"Unmatched",
			"EOF",
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
			"Multiply",
			"Divide",
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
			"ExpE",
			"StmtE",
			"Plus",
			"Minus",
			"Percentage",
			"CommentE",
			"Padding",
			"Newline",
			"ExpB",
			"StmtB",
			"CommentB",
			"Raw",
		},
		{
			std::regex(""), // Unmatched
			std::regex(""), // Eof,
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
			std::regex("\\*"), // Mult
			std::regex("\\/"), // Div
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
			std::regex("(-|\\+)?\\}\\}"), // ExpE
			std::regex("(-|\\+)?%\\}"), // StmtE
			std::regex("\\+"), // Plus
			std::regex("\\-"), // Minus
			std::regex("%"), // Perc
			std::regex("(-|\\+)?#\\}"), // CommentE
			std::regex("([ \\t])+"), // Pad
			std::regex("\\n"), // Newline
			std::regex("\\{\\{(-|\\+)?"), // ExpB
			std::regex("\\{%(-|\\+)?"), // StmtB
			std::regex("\\{#(-|\\+)?"), // CommentB
			std::regex("([^\\s\\{]|(\\{[^\\{#%\\s])|(\\{(?=\\s)))+"), // Raw
		},
		true,
	};

	bool _is_b_type(int type) {
		return type >= int(TemplTokenType::ExpB) && type <= int(TemplTokenType::CommentB);
	}

	bool _is_e_type(int type) {
		return type >= int(TemplTokenType::ExpE) && type <= int(TemplTokenType::CommentE);
	}

	/**
	 * Combines every token not in a statement into an unmatched token
	 * Removes padding before and after statements and comments
	 */
	std::vector<Token> _simplify_templ_tokens(std::vector<Token> const &tokens) {
		auto result = std::vector<Token>();
		using T = TemplTokenType;

		bool in_statement = false;
		auto cur_token = Token();
		int skipped_pad = 0;
		for (auto i = 0; i < tokens.size(); i++) {
			auto &t = tokens[i];

			//Handle remove padding before and after
			if (_is_b_type(t.type())) {
				Token const *close_tag = &t;
				while (!_is_e_type(close_tag->type())) {
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

			if (!in_statement && skipped_pad > 0 && (t.type() == int(T::Pad) || t.type() == int(T::Newline))) {
				skipped_pad--;
			} else {
				result.push_back(t);
			}

			if (_is_b_type(t.type())) {
				in_statement = true;
			}

			if (_is_e_type(t.type())) {
				in_statement = false;
			}
		}

		return result;
	}

	std::vector<Token> tokenize_templ(util::StringRef c) {
		auto result = std::vector<Token>();
		auto &config = TEMPL_TOK_CONFIG;
		bool syntax_mode = false;
		while (*c) {
			int min, max;
			if (syntax_mode) {
				min = int(TemplTokenType::If);
				max = int(TemplTokenType::Newline);
			} else {
				min = int(TemplTokenType::CommentE);
				max = int(TemplTokenType::Raw);
			}
			int type;
			for (type = min; type <= max; type++) {
				auto &rule = config.parse_table[type];
				auto match = std::cmatch();
				auto flags = std::regex_constants::match_continuous
					| std::regex_constants::match_not_null;
				if (std::regex_search(c.str().begin(), c.str().end(), match, rule, flags)) {
					result.push_back(Token(type, c.substr(0, match.length())));
					c += match.length();
					break;
				}
			}
			if (type > max) {
				//TODO: throw an error
				log_error() << "Couldn't recognize token " << c.substr(0, 5).str() << "..." << std::endl;
				break;
			} else if (type == int(TemplTokenType::ExpB) || type == int(TemplTokenType::StmtB)) {
				// Don't switch to syntax mode for comments
				syntax_mode = true;
			} else if (type == int(TemplTokenType::ExpE) || type == int(TemplTokenType::StmtE)) {
				syntax_mode = false;
			}
		}
		result.push_back(Token(int(Token::Type::Eof), c));

		result = _simplify_templ_tokens(result);

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

