#pragma once

#include <vector>

#include "util/StringRef.hpp"
#include "util/result.hpp"
#include "AstNode.hpp"
#include "CfgContext.hpp"
#include "Tokenizer.hpp"

namespace cg {
	class Parser {
		public:
			using Ptr = std::unique_ptr<Parser>;

			virtual ~Parser() {}

			inline virtual util::Result<size_t, KError> match(
				std::vector<Token> const &tokens
			) {
				try {
					return parse(tokens)->consumed_all().size();
				} catch_kerror;
			}
			inline virtual util::Result<size_t, KError> match(
				util::StringRef str
			) {
				try {
					return match(simplify_tokens(tokenize(str)));
				} catch_kerror;
			}


			virtual util::Result<AstNode, KError> parse(
				std::vector<Token> const &tokens
			) = 0;
			inline virtual util::Result<AstNode, KError> parse(
				util::StringRef str
			) {
				try {
					return parse(simplify_tokens(tokenize(str)));
				} catch_kerror;
			}

			virtual CfgContext const &cfg() const = 0;
			virtual CfgContext &cfg() = 0;
	};
}
