#pragma once

#include <string>
#include <vector>

#include "Error.hpp"
#include "CfgContext.hpp"
#include "TemplObj.hpp"
#include "AstNode.hpp"
#include "Parser.hpp"
#include "AbsoluteSolver.hpp"
#include "util/result.hpp"

namespace cg {
	/**
	 * @brief Template based code gen engine
	 */
	class TemplGen {
		public:
			TemplGen() = default;

			TemplGen(TemplGen const &other) = delete;
			TemplGen(TemplGen &&other);

			TemplGen &operator=(TemplGen const &other) = delete;
			TemplGen &operator=(TemplGen &&other);

			static util::Result<std::string, Error> codegen(
				std::string const &str,
				TemplObj const &args,
				std::string const &filename = "codegen"
			);

			static util::Result<std::string, Error> codegen(
				std::string const &str,
				TemplDict const &args,
				std::string const &filename = "codegen"
			);

			CfgContext const &cfg() const { return _parser->cfg(); }
			CfgContext &cfg() { return _parser->cfg(); }
		private:
			static util::Result<void, Error> _setup_parser();
			static Parser::Ptr _parser;
			ParserContext _parser_result;

		private:
			using CodegenRes = util::Result<std::string, Error>;
			using EvalRes = util::Result<TemplObj, Error>;

			CodegenRes _codegen(AstNode const &node, TemplDict &args);

			CodegenRes _cg_default(AstNode const &node, TemplDict &args);
			CodegenRes _cg_recursive(AstNode const &node, TemplDict &args);
			CodegenRes _cg_ref(AstNode const &node, TemplDict &args, size_t count=1);

			CodegenRes _cg_identifier(AstNode const &node, TemplDict &args);
			CodegenRes _cg_line(AstNode const &node, TemplDict &args);
			CodegenRes _cg_lines(AstNode const &node, TemplDict &args);

			CodegenRes _cg_comment(AstNode const &node, TemplDict &args);

			CodegenRes _cg_expression(AstNode const &node, TemplDict &args);

			CodegenRes _cg_statement(AstNode const &node, TemplDict &args);
			CodegenRes _cg_sif(AstNode const &node, TemplDict &args);
			CodegenRes _cg_sfor(AstNode const &node, TemplDict &args);
			util::Result<void, Error> _cg_smacro(AstNode const &node, TemplDict &args);
			CodegenRes _cg_sinclude(AstNode const &node, TemplDict &args);

			EvalRes _eval(util::Result<AstNode, Error> const &node, TemplDict const &args);
			EvalRes _eval(AstNode const &node, TemplDict const &args);

			EvalRes _eval_exp_sing(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp_id(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp_int(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp_str(AstNode const &node, TemplDict const &args);

			EvalRes _eval_exp1(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp_member(
				TemplObj const &lhs,
				AstNode const &node,
				TemplDict const &args
			);
			EvalRes _eval_exp_call(
				TemplObj const &lhs,
				AstNode const &node,
				TemplDict const &args
			);

			EvalRes _eval_exp2(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp3(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp4(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp6(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp7(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp11(AstNode const &node, TemplDict const &args);
			EvalRes _eval_exp12(AstNode const &node, TemplDict const &args);
			EvalRes _eval_filter_frag(
				TemplObj const &lhs,
				AstNode const &node,
				TemplDict const &args
			);
			EvalRes _eval_filter(AstNode const &node, TemplDict const &args);

			util::Result<bool, Error> _tag_keep_padding(AstNode const &node, bool def);

			util::Result<void, Error> _add_builtin_identifier(
				std::string const &name,
				TemplObj const &func,
				TemplDict &args
			);
			util::Result<void, Error> _add_builtin_identifiers(TemplDict &args);
	};
}
