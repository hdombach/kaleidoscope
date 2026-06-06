#pragma once

/* Sub-program for generating code-generated document interfaces */

#include "codegen/AbsoluteSolver.hpp"
#include "codegen/ParserContext.hpp"
#include "Tokenizer.hpp"
#include "util/result.hpp"
#include "Error.hpp"
#include "Validate.hpp"

namespace serial {
	class DocEngine {
		public:
			DocEngine() = default;

			util::Result<void, Error> load(
				std::vector<std::string> const &files,
				std::string const &out_dir
			);

		private:
			static cg::Parser::Ptr _parser;
			static util::Result<void, Error> _setup_parser();

			cg::ParserContext _parser_ctx = cg::ParserContext(TOK_CONFIG);
			std::map<std::string, cg::AstNode*> _roots;
			Document _doc;
	};
}
