#pragma once

#include <string>
#include <vector>

#include "CfgContext.hpp"
#include "TemplObj.hpp"
#include "AstNode.hpp"
#include "util/result.hpp"
#include "util/errors.hpp"

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

			util::Result<std::string, KError> _codegen_default(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _codegen_identifier(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _codegen_exp_id(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _codegen_comment(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;
			util::Result<std::string, KError> _codegen_expression(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

			util::Result<std::string, KError> _codegen_file(
				AstNode const &node,
				TemplObj::Dict const &args
			) const;

	};
}
