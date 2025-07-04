#pragma once

#include <map>
#include <string>

#include "util/StringRef.hpp"
#include "Tokenizer.hpp"
#include "AstNode.hpp"


namespace cg {
	/**
	 * @brief The result of a parse along with resources that need to have the same
	 * scope.
	 * This makes sure points to assets like the file source code remain valid
	 */
	class ParserContext {
		public:
			std::vector<Token> const &get_tokens(util::StringRef str);
			AstNode &create_tok_node(Token const &token);
			AstNode &create_rule_node(std::string const &cfg_name);
			AstNode &create_node();

		private:
			struct FileItem {
				std::string source;
				std::vector<Token> tokens;
			};

			uint32_t _node_id=0;
			uint32_t _node_count;
			uint32_t _bank_count=100;
			std::map<std::string, FileItem> _items;
			std::vector<std::vector<AstNode>> _node_bank;

	};
}
