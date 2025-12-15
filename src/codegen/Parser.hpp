#pragma once

#include <vector>

#include "util/StringRef.hpp"
#include "util/result.hpp"
#include "AstNode.hpp"
#include "CfgContext.hpp"
#include "Tokenizer.hpp"
#include "ParserContext.hpp"
#include "Error.hpp"

namespace cg {
	class Parser {
		public:
			using Ptr = std::unique_ptr<Parser>;
			virtual ~Parser() {}

			inline virtual util::Result<size_t, Error> match(
				util::StringRef const &str
			) {
				auto r = ParserContext();
				if (auto consumed = parse(str, r)) {
					return consumed.value()->consumed_all().size();
				} else {
					return Error(ErrorType::INVALID_PARSE, "Could not match", consumed.error());
				}
			}

			virtual util::Result<AstNode*, Error> parse(
				util::StringRef const &str,
				ParserContext &result
			) = 0;

			virtual CfgContext const &cfg() const = 0;
			virtual CfgContext &cfg() = 0;
	};
}
