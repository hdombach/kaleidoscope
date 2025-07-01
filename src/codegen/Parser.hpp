#pragma once

#include <vector>

#include "util/StringRef.hpp"
#include "util/result.hpp"
#include "AstNode.hpp"
#include "CfgContext.hpp"
#include "Tokenizer.hpp"
#include "ParserResult.hpp"

namespace cg {
	class Parser {
		public:
			using Ptr = std::unique_ptr<Parser>;
			virtual ~Parser() {}

			inline virtual util::Result<size_t, KError> match(
				util::StringRef const &str
			) {
				try {
					return parse(str)->root_node().consumed_all().size();
				} catch_kerror;
			}

			virtual util::Result<ParserResult, KError> parse(
				util::StringRef const &str
			) = 0;

			virtual CfgContext const &cfg() const = 0;
			virtual CfgContext &cfg() = 0;
	};
}
