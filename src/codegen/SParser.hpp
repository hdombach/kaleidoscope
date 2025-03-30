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
			struct Stack {
				util::StringRef str;
				std::string rule;
				Stack const *parent = nullptr;


				Stack() = default;
				Stack(util::StringRef const &s, std::string const &r, Stack const *p):
					str(s), rule(r), parent(p)
				{}

				Stack child(size_t offset) const {
					return {str+offset, rule, this};
				}

				Stack child(size_t offset, std::string const &new_rule) const {
					return {str+offset, new_rule, this};
				}
			};
		public:
			SParser(CfgContext const &ctx);

			/**
			 * @brief Matches a string against a CfgRuleSet specified by name
			 * @param[in] str String to match against
			 * @param[in] root_node Name of CfgRuleSet to match against
			 * @returns Number of characters consumed
			 */
			util::Result<size_t, KError> match(
				std::string const &str,
				std::string const &root_node
			);

			/**
			 * @brief Generates abstract syntax tree
			 * @param[in] str String to generate from
			 * @param[in] root_node Name of CfgRuleSet to generate with
			 * @returns Generated abstract syntax tree
			 */
			util::Result<AstNode, KError> parse(
				std::string const &str,
				std::string const &root_node,
				std::string const &filename = "codegen"
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
			util::Result<AstNode, KError> _parse(Stack const &stack, CfgRuleSet const &set);
			util::Result<AstNode, KError> _parse(Stack const &stack, CfgRule const &rule);
			util::Result<AstNode, KError> _parse(Stack const &stack, CfgLeaf const &leaf);
	};
}
