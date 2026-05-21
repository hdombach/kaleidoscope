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
			ParserContext() = default;
			ParserContext(Token::Config const &tok_config);

			void destroy();

			std::vector<Token> const &get_tokens(util::StringRef str);
			AstNode &create_tok_node(Token const &token);
			AstNode &create_rule_node(std::string const &cfg_name);
			AstNode &create_node();

			Token::Config const &tok_config() const;

		private:
			struct FileItem {
				std::string source;
				std::vector<Token> tokens;
			};

			/**
			 * @brief The next available uid
			 */
			uint32_t _node_id=0;
			/**
			 * @brief The current number of allocated nodes
			 */
			uint32_t _node_count = 0;
			/**
			 * @brief The size of a single node bank
			 */
			uint32_t _bank_count=100;
			/**
			 * @brief The parsed tokens across all files
			 */
			std::map<std::string, FileItem> _items;
			/**
			 * @brief The allocated nodes
			 */
			std::vector<std::vector<AstNode>> _node_bank;

			Token::Config const *_tok_config = nullptr;

	};
}
