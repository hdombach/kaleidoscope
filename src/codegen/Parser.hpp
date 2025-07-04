#pragma once

#include <vector>

#include "util/StringRef.hpp"
#include "util/result.hpp"
#include "AstNode.hpp"
#include "CfgContext.hpp"
#include "Tokenizer.hpp"
#include "ParserContext.hpp"

namespace cg {
	class Parser {
		public:
			using Ptr = std::unique_ptr<Parser>;
			virtual ~Parser() {}

			inline virtual util::Result<size_t, KError> match(
				util::StringRef const &str
			) {
				try {
					auto r = ParserContext();
					return parse(str, r).value()->consumed_all().size();
				} catch_kerror;
			}

			virtual util::Result<AstNode*, KError> parse(
				util::StringRef const &str,
				ParserContext &result
			) = 0;

			virtual CfgContext const &cfg() const = 0;
			virtual CfgContext &cfg() = 0;
	};
}
