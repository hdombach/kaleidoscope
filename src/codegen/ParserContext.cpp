#include "ParserContext.hpp"
#include "codegen/TemplTokenizer.hpp"
#include "util/log.hpp"

namespace cg {
	ParserContext::ParserContext(Token::Config const &tok_config):
		_tok_config(&tok_config)
	{ }

	void ParserContext::destroy() {
		_node_id = 0;
		_node_count = 0;
		_items.clear();
		_node_bank.clear();
	}

	std::vector<Token> const &ParserContext::get_tokens(util::StringRef str) {
		auto &item = _items[str.location().file_name];
		if (item.source.empty()) {
			item.source = str.str();
			item.tokens = simplify_templ_tokens(tokenize({item.source.c_str(), str.location().file_name.c_str()}, *_tok_config));
		}
		return item.tokens;
	}

	AstNode &ParserContext::create_tok_node(Token const &token) {
		auto &n = create_node();
		n = AstNode::create_tok(++_node_id, token, *this);
		return n;
	}

	AstNode &ParserContext::create_rule_node(std::string const &cfg_name) {
		auto &n = create_node();
		n = AstNode::create_rule(++_node_id, cfg_name, *this);
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

	Token::Config const &ParserContext::tok_config() const {
		return *_tok_config;
	}
}
