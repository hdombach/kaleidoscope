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

		private:
			struct FileItem {
				std::string source;
				std::vector<Token> tokens;
			};

			std::map<std::string, FileItem> _items;
	};
}
