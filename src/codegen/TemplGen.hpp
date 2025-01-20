#pragma once

#include <string>
#include <vector>

#include "CfgContext.hpp"
#include "TemplObj.hpp"
#include "AstNode.hpp"
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
				TemplObj::Dict const &args
			) const;
		private:
			CfgContext _ctx;
			std::vector<std::string> _prims;

		private:
			util::Result<std::string, KError> _codegen(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

			util::Result<std::string, KError> _cg_default(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _cg_ref(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

			util::Result<std::string, KError> _cg_identifier(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _cg_line(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _cg_lines(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

			util::Result<std::string, KError> _cg_comment(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

			util::Result<std::string, KError> _cg_expression(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

			util::Result<std::string, KError> _cg_statement(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _cg_sif(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _cg_sfor(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

			util::Result<TemplObj, KError> _eval(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<TemplObj, KError> _eval_exp_id(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;


			util::Result<bool, KError> _tag_keep_padding(AstNode const &node, bool def) const;

	};
}
