#pragma once

#include <cstdint>

#include "util/StringRef.hpp"
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
			struct Stack {
				util::StringRef str;
				CfgNode const &node;
				Stack *parent = nullptr;
			};
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
			/**
			 * @brief Most recent failure (even if it isn't officially error yet)
			 */
			KError _last_failure;

		private:
			/**
			 * @brief Updates last failure if it is newer
			 * @returns the failure passed in
			 */
			KError _set_failure(KError const &failure);
			/***********************************
			 * Parser helper functions
			 * *********************************/
			util::Result<AstNode, KError> _parse(Stack stack);
			util::Result<AstNode, KError> _parse_lit(Stack stack);
			util::Result<AstNode, KError> _parse_ref(Stack stack);
			util::Result<AstNode, KError> _parse_seq(Stack stack);
			util::Result<AstNode, KError> _parse_alt(Stack stack);
			util::Result<AstNode, KError> _parse_cls(Stack stack);
			util::Result<AstNode, KError> _parse_opt(Stack stack);
			util::Result<AstNode, KError> _parse_neg(Stack stack);
	};
}
