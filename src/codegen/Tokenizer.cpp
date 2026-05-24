#include <regex>

#include "Tokenizer.hpp"
#include "util/StringRef.hpp"
#include "util/format.hpp"
#include "util/Util.hpp"

namespace cg {
	Token::Token(int type, util::StringRef const &ref):
		_type(type),
		_str(ref.str()),
		_loc(ref.location())
	{}

	int Token::type() const { return _type; }
	std::string Token::content() const { return _str; }
	util::FileLocation Token::loc() const { return _loc; }
	std::string Token::debug_str(Config const &config) const {
		const char *name = "UNKNOWN";
		if (_type < 0 || _type >= config.name_table.size()) {
			name = config.name_table[_type].c_str();
		}
		return util::f("(", config.name_table[_type], " \"", util::escape_str(content()), "\")");
	}

	void Token::concat(Token const &t) {
		if (!exists()) {
			*this = t;
			_type = int(Type::Unmatched);
			return;
		}
		_str += t._str;
	}

	bool Token::exists() const {
		return !_str.empty();
	}

	Token &Token::operator+=(Token const &rhs) {
		*this = *this + rhs;
		return *this;
	}

	std::vector<Token> tokenize(util::StringRef c, Token::Config const &config) {
		auto result = std::vector<Token>();
		while (*c) {
			auto type = int(Token::Type::Unmatched);
			//std::regex_search(test, reg, std::regex_constants::match_continuous | std::regex_constants::match_not_null)
			for (auto &rule : config.parse_table) {
				auto match = std::cmatch();
				auto flags = std::regex_constants::match_continuous
					| std::regex_constants::match_not_null;
				if (std::regex_search(c.str().begin(), c.str().end(), match, rule, flags)) {
					result.push_back(Token(type, c.substr(0, match.length())));
					c += match.length();
					break;
				}
				type = (type + 1) % config.size();
			}
			if (type == int(Token::Type::Unmatched)) {
				result.push_back(Token(type, c.substr(0, 1)));
				c += 1;
			}
		}
		result.push_back(Token(int(Token::Type::Eof), c));
		return result;
	}
}

