#pragma once

#include <regex>
#include <iostream>
#include "util/FileLocation.hpp"
#include "util/StringRef.hpp"


namespace cg {
	class Token {
		public:
			struct Config {
				std::vector<std::string> name_table;
				std::vector<std::regex> parse_table;

				size_t size() const { return parse_table.size(); }
			};

			enum class Type: int {
				Unmatched,
				Eof,
			};

			Token() = default;
			Token(int type, util::StringRef const &ref);

			int type() const;
			std::string content() const;
			util::FileLocation loc() const;
			std::string debug_str(Config const &config) const;
			void concat(Token const &t);

			bool exists() const;

			Token &operator+=(Token const &rhs);
		private:
			int _type=int(Type::Unmatched);
			std::string _str;
			util::FileLocation _loc;

			/*
			 * I originaly used a StringRef instead of a string to reduce extra allocations.
			 * However, it cause issues when including macros from a file.
			 * Macros would keep AstNodes after the string it was parsed from was deallocated
			 * leading to invalid references for the StringRefs.
			 */
	};

	inline Token operator+(Token const &lhs, Token const &rhs) {
		auto result = lhs;
		result.concat(rhs);
		return result;
	}

	std::vector<Token> tokenize(util::StringRef str, Token::Config const &config);
}

