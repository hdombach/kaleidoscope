#pragma once

#include <regex>
#include <iostream>
#include "util/StringRef.hpp"

namespace cg {
	class Token {
		public:
			enum Type: int {
				Unmatched,
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
				Eof,
			};

			Token() = default;
			Token(Type type, util::StringRef const &ref);

			Type type() const;
			std::string content() const;
			std::string str() const;
			util::StringRef str_ref() const;
			static const char *type_str(Type type);
			void concat(Token const &t);

			bool exists() const;
			operator bool() const;

			Token &operator+=(Token const &rhs);
		private:
			Type _type=Type::Unmatched;
			util::StringRef _str=util::StringRef();
	};

	inline std::ostream &operator<<(std::ostream &os, Token const &t) {
		return os << t.str();
	}

	inline std::ostream &operator<<(std::ostream &os, Token::Type const &t) {
		return os << Token::type_str(t);
	}

	inline Token operator+(Token const &lhs, Token const &rhs) {
		auto result = lhs;
		result.concat(rhs);
		return result;
	}

	std::vector<Token> tokenize(util::StringRef str);

	/**
	 * Combines every token not in a statement into an unmatched token
	 */
	std::vector<Token> simplify_tokens(std::vector<Token> const &tokens);
}
