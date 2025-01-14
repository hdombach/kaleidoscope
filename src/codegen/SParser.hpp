#pragma once

#include <cstdint>

#include "util/result.hpp"
#include "CfgContext.hpp"
#include "AstNode.hpp"

namespace cg {
	/*
	 * @brief A Simple ast Parser
	 */
	class SParser {
		public:
			using Type = CfgNode::Type;
		public:
			SParser(CfgContext const &ctx);

			/**
			 * @brief Matches a string against a CfgNode specified by name
			 * @param[in] str String to match against
			 * @param[in] root_node Name of CfgNode to match against
			 * @returns Number of characters consumed
			 */
			util::Result<size_t, KError> match(
				std::string const &str,
				std::string const &root_node
			);

			/**
			 * @brief Generates abstract syntax tree
			 * @param[in] str String to generate from
			 * @param[in] root_node Name of CfgNode to generate with
			 * @returns Generated abstract syntax tree
			 */
			util::Result<AstNode, KError> parse(
				std::string const &str,
				std::string const &root_node
			);

		private:
			/**
			 * @brief uid for constructing AstNodes
			 */
			uint32_t _uid;
			CfgContext const &_ctx;

		private:
			/************************************
			 * Match helper functions
			 ************************************/
			util::Result<size_t, KError> _match(const char *str, CfgNode const &node);
			util::Result<size_t, KError> _match_lit(const char *str, CfgNode const &node);
			util::Result<size_t, KError> _match_ref(const char *str, CfgNode const &node);
			util::Result<size_t, KError> _match_seq(const char *str, CfgNode const &node);
			util::Result<size_t, KError> _match_alt(const char *str, CfgNode const &node);
			util::Result<size_t, KError> _match_cls(const char *str, CfgNode const &node);
			util::Result<size_t, KError> _match_opt(const char *str, CfgNode const &node);
			util::Result<size_t, KError> _match_neg(const char *str, CfgNode const &node);

			/***********************************
			 * Parser helper functions
			 * *********************************/
			util::Result<AstNode, KError> _parse(const char *str, CfgNode const &node);
			util::Result<AstNode, KError> _parse_lit(const char *str, CfgNode const &node);
			util::Result<AstNode, KError> _parse_ref(const char *str, CfgNode const &node);
			util::Result<AstNode, KError> _parse_seq(const char *str, CfgNode const &node);
			util::Result<AstNode, KError> _parse_alt(const char *str, CfgNode const &name);
			util::Result<AstNode, KError> _parse_cls(const char *str, CfgNode const &name);
			util::Result<AstNode, KError> _parse_opt(const char *str, CfgNode const &name);
			util::Result<AstNode, KError> _parse_neg(const char *str, CfgNode const &name);
	};
}
