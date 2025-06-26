#pragma once

#include <cstdint>

#include "util/StringRef.hpp"
#include "util/result.hpp"
#include "CfgContext.hpp"
#include "AstNode.hpp"
#include "Parser.hpp"

namespace cg {
	/*
	 * @brief A Simple ast Parser
	 */
	class SParser: public Parser {
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
			SParser() = default;
			static SParser::Ptr create(std::unique_ptr<CfgContext> &&ctx);

			SParser(SParser const &other) = delete;
			SParser(SParser &&other);
			SParser &operator=(SParser const &other) = delete;
			SParser &operator=(SParser &&other);

			~SParser() {}

			/**
			 * @brief Generates abstract syntax tree
			 * @param[in] str String to generate from
			 * @param[in] root_node Name of CfgRuleSet to generate with
			 * @returns Generated abstract syntax tree
			 */
			util::Result<AstNode, KError> parse(
				std::vector<Token> const &tokens
			) override;

			CfgContext const &cfg() const override;
			CfgContext &cfg() override;

		private:
			/**
			 * @brief uid for constructing AstNodes
			 */
			uint32_t _uid;
			CfgContext::Ptr _ctx;
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
			util::Result<AstNode, KError> _parse(
				std::vector<Token> const &tokens,
				uint32_t i,
				CfgRuleSet const &set
			);
			util::Result<AstNode, KError> _parse(
				std::vector<Token> const &tokens,
				uint32_t i,
				CfgRule const &rule,
				std::string const &set_name
			);
			util::Result<AstNode, KError> _parse(
				std::vector<Token> const &tokens,
				uint32_t i,
				CfgLeaf const &leaf
			);
	};
}
