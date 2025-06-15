#pragma once

#include "util/StringRef.hpp"
#include "util/result.hpp"
#include "AstNode.hpp"
#include "CfgContext.hpp"

namespace cg {
	class Parser {
		public:
			using Ptr = std::unique_ptr<Parser>;

			virtual ~Parser() {}

			inline virtual util::Result<size_t, KError> match(
				std::string const &str,
				std::string const &filename = "codegen"
			) {
				try {
					auto ref = util::StringRef(str.c_str(), filename.c_str());
					return parse(str, filename)->size();
				} catch_kerror;
			}
			virtual util::Result<AstNode, KError> parse(
				std::string const &str,
				std::string const &filename = "codegen"
			) = 0;

			virtual CfgContext const &cfg() const = 0;
			virtual CfgContext &cfg() = 0;
	};
}
