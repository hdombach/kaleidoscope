#include <regex>

#include "Tokenizer.hpp"
#include "util/StringRef.hpp"
#include "util/format.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"

namespace cg {
	Token::Token(Type type, util::StringRef const &ref):
		_type(type),
		_str(ref)
	{}

	Token::Type Token::type() const { return _type; }
	std::string Token::content() const { return _str.str(); }
	std::string Token::str() const {
		return util::f("(", type_str(_type), " \"", util::escape_str(content()), "\")");
	}
	util::StringRef Token::str_ref() const {
		return _str;
	}
	const char *Token::type_str(Type type) {
		const char *names[] = {
			"Unmatched",
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
			"EOF",
		};
		return names[type];
	}

	void Token::concat(Token const &t) {
		if (!exists()) {
			*this = t;
			_type = Type::Unmatched;
			return;
		}
		_str += t._str;
	}

	bool Token::exists() const {
		return !_str.empty();
	}

	Token::operator bool() const {
		return exists();
	}

	Token &Token::operator+=(Token const &rhs) {
		*this = *this + rhs;
		return *this;
	}

	std::vector<std::regex> _token_table{
		std::regex(""), // Unmatched
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
		std::regex(""),
	};

	std::vector<Token> tokenize(util::StringRef c) {
		auto result = std::vector<Token>();
		while (*c) {
			auto type = Token::Type::Unmatched;
			//std::regex_search(test, reg, std::regex_constants::match_continuous | std::regex_constants::match_not_null)
			for (auto &rule : _token_table) {
				auto match = std::cmatch();
				auto flags = std::regex_constants::match_continuous
					| std::regex_constants::match_not_null;
				if (std::regex_search(c.str().c_str(), match, rule, flags)) {
					result.push_back(Token(type, c.substr(0, match.length())));
					c += match.length();
					break;
				}
				type = Token::Type((type + 1) % _token_table.size());
			}
			if (type == Token::Type::Unmatched) {
				result.push_back(Token(type, c.substr(0, 1)));
				c += 1;
			}
		}
		result.push_back(Token(Token::Type::Eof, c));
		return result;
	}

	std::vector<Token> simplify_tokens(const std::vector<Token> &tokens) {
		auto result = std::vector<Token>();

		bool in_statement = false;
		auto cur_token = Token();
		for (auto t : tokens) {
			if (in_statement) {
				result.push_back(t);
				if (
					t.type() == Token::ExpE
					|| t.type() == Token::StmtE
				) {
					in_statement = false;
				}
			} else {
				if (
					t.type() == Token::Pad
					|| t.type() == Token::Newline
					|| t.type() == Token::Eof
					|| t.type() == Token::CommentB
					|| t.type() == Token::CommentE
				) {
					if (cur_token.exists())
						result.push_back(cur_token);
					cur_token = Token();
					result.push_back(t);
				} else if (
					t.type() == Token::ExpB
					|| t.type() == Token::StmtB
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
