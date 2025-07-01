#include "ParserResult.hpp"
#include "codegen/Tokenizer.hpp"

namespace cg {
	std::vector<Token> const &ParserResult::get_tokens(util::StringRef str) {
		auto &item = _items[str.location().file_name];
		if (item.source.empty()) {
			item.source = str.str();
			item.tokens = simplify_tokens(tokenize({item.source.c_str(), str.location().file_name.c_str()}));
		}
		return item.tokens;
	}
}
