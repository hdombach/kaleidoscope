#pragma once

#include <string>
#include <vector>

#include "CfgContext.hpp"
#include "TemplObj.hpp"
#include "AstNode.hpp"
#include "SParser.hpp"
#include "util/result.hpp"
#include "util/KError.hpp"

namespace cg {
	/**
	 * @brief Template based code gen engine
	 */
	class TemplGen {
		public:
			static util::Result<TemplGen, KError> create();

			util::Result<std::string, KError> codegen(
				std::string const &str,
				TemplObj const &args,
				std::string const &filename = "codegen"
			) const;

			util::Result<std::string, KError> codegen(
				std::string const &str,
				TemplDict const &args,
				std::string const &filename = "codegen"
			) const;
		private:
			CfgContextTemp _ctx;

		private:
			using CodegenRes = util::Result<std::string, KError>;
			using EvalRes = util::Result<TemplObj, KError>;

			CodegenRes _codegen(AstNode const &node, TemplDict &args, SParser &parser) const;

			CodegenRes _cg_default(AstNode const &node, TemplDict &args, SParser &parser) const;
			CodegenRes _cg_recursive(AstNode const &node, TemplDict &args, SParser &parser) const;
			CodegenRes _cg_ref(AstNode const &node, TemplDict &args, SParser &sparser) const;

			CodegenRes _cg_identifier(AstNode const &node, TemplDict &args, SParser &sparser) const;
			CodegenRes _cg_line(AstNode const &node, TemplDict &args, SParser &sparser) const;
			CodegenRes _cg_lines(AstNode const &node, TemplDict &args, SParser &sparser) const;

			CodegenRes _cg_comment(AstNode const &node, TemplDict &args, SParser &parser) const;

			CodegenRes _cg_expression(AstNode const &node, TemplDict &args, SParser &parser) const;

			CodegenRes _cg_statement(AstNode const &node, TemplDict &args, SParser &parser) const;
			CodegenRes _cg_sif(AstNode const &node, TemplDict &args, SParser &parser) const;
			CodegenRes _cg_sfor(AstNode const &node, TemplDict &args, SParser &parser) const;
			util::Result<void, KError> _cg_smacro(AstNode const &node, TemplDict &args, SParser &parser) const;
			CodegenRes _cg_sinclude(AstNode const &node, TemplDict &args, SParser &parser) const;

			EvalRes _eval(util::Result<AstNode, KError> const &node, TemplDict const &args) const;
			EvalRes _eval(AstNode const &node, TemplDict const &args) const;

			EvalRes _eval_exp_sing(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp_id(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp_int(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp_str(AstNode const &node, TemplDict const &args) const;

			EvalRes _eval_exp1(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp_member(
				TemplObj const &lhs,
				AstNode const &node,
				TemplDict const &args
			) const;
			EvalRes _eval_exp_call(
				TemplObj const &lhs,
				AstNode const &node,
				TemplDict const &args
			) const;

			EvalRes _eval_exp2(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp3(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp4(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp6(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp7(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp11(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_exp12(AstNode const &node, TemplDict const &args) const;
			EvalRes _eval_filter_frag(
				TemplObj const &lhs,
				AstNode const &node,
				TemplDict const &args
			) const;
			EvalRes _eval_filter(AstNode const &node, TemplDict const &args) const;

			util::Result<bool, KError> _tag_keep_padding(AstNode const &node, bool def) const;

			util::Result<void, KError> _add_builtin_identifier(
				std::string const &name,
				TemplObj const &func,
				TemplDict &args
			) const;
			util::Result<void, KError> _add_builtin_identifiers(TemplDict &args) const;
	};
}
