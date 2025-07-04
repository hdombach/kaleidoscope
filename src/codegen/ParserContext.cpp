#include "ParserContext.hpp"
#include "codegen/Tokenizer.hpp"
#include "util/log.hpp"

namespace cg {
	std::vector<Token> const &ParserContext::get_tokens(util::StringRef str) {
		auto &item = _items[str.location().file_name];
		if (item.source.empty()) {
			item.source = str.str();
			item.tokens = simplify_tokens(tokenize({item.source.c_str(), str.location().file_name.c_str()}));
		}
		return item.tokens;
	}

	AstNode &ParserContext::create_tok_node(Token const &token) {
		auto &n = create_node();
		n = AstNode::create_tok(++_node_id, token);
		return n;
	}

	AstNode &ParserContext::create_rule_node(std::string const &cfg_name) {
		auto &n = create_node();
		n = AstNode::create_rule(++_node_id, cfg_name);
		return n;
	}

	AstNode &ParserContext::create_node() {
		while (_node_count >= _node_bank.size() * 100) {
			_node_bank.push_back(std::vector<AstNode>(100));
		}
		auto &n = _node_bank[_node_count / 100][_node_count % 100];
		_node_count++;
		return n;
	}
}
