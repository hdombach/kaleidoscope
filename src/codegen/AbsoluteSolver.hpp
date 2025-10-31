#pragma once

#include <ostream>
#include <variant>
#include <vector>
#include <set>

#include "util/FileLocation.hpp" // Needed for operator resolution
#include "codegen/CfgContext.hpp"
#include "util/IterAdapter.hpp"
#include "util/result.hpp"
#include "AstNode.hpp"
#include "AbsoluteTable.hpp"
#include "Parser.hpp"

namespace cg::abs {
	/**
	 * Can either be a node being consructed or a state in the AbsoluteTable
	 */
	class StackElement {
		public:
			StackElement();
			StackElement(StackElement const &other);
			StackElement(StackElement &&other);
			StackElement(AstNode &node);
			StackElement(uint32_t table_state);

			StackElement &operator=(StackElement const &other);
			StackElement &operator=(StackElement &&other);

			bool is_table_state() const;
			uint32_t table_state() const;
			bool is_node() const;
			AstNode const &node() const;
			AstNode &node();

			std::ostream &debug(std::ostream &os) const;

		private:
			std::variant<AstNode*, uint32_t> _value;
	};

	/**
	 * @brief A (hopefully) fast parser for the context free grammar.
	 * It uses an LR(1) parser I think.
	 *
	 * The core idea is to generate a table. Each row corresponds to a state
	 * and each column corresponds to either a character or what state you came from.
	 * Each state corresponds to a set of Cfg rules with a position associated
	 * with each one.
	 *
	 * This means the context free grammar needs to be modified to not use string
	 * constants. (Only use character sets).
	 *
	 * The AbsoluteSolver is meant to be setup using a provided context free grammar.
	 * Once created, a parser can parse any number of strings and uses a ParserContext
	 * to pass lifetime of generated AST to caller
	 */
	class AbsoluteSolver: public Parser {
		public:
			using Ptr = std::unique_ptr<AbsoluteSolver>;

			AbsoluteSolver() = default;
			~AbsoluteSolver() {}

			static util::Result<AbsoluteSolver::Ptr, KError> create(
				CfgContext::Ptr &&ctx
			);

			void print_table(std::ostream &os);

			using Parser::match;
			using Parser::parse;

			util::Result<AstNode*, KError> parse(
				util::StringRef const &str,
				ParserContext &parser_ctx
			) override;

			CfgContext const &cfg() const override;
			CfgContext &cfg() override;

		public:
			using State = util::IterAdapter<uint32_t*>;

			struct RuleGroup {
				CfgLeaf leaf;
				TableState rules;
			};

		private:
			CfgContext::Ptr _ctx;
			AbsoluteTable _table;
			std::vector<CfgRule> _rules;

		private:
			/**
			 * @brief Reduces the top of the stack using the provided rule_id
			 * @param[in, out] stack The stack to reduce
			 * @param[in] rule_id
			 * @param[in, out] node_id uid to assign the node
			 */
			void _reduce(
				std::vector<StackElement> &stack,
				uint32_t rule_id,
				uint32_t &node_id,
				ParserContext &parser_ctx
			);

			/**
			 * @brief Creates a state for a given rule
			 * @param[in] state_rule
			 * @returns Index in _states of created state
			 */
			uint32_t _add_state(TableState const &table_state);

			/**
			 * @brief Get a rule for a corresponding unique_id
			 * The id 0 is reserved for null
			 */
			uint32_t _get_rule_id(RulePos const &pos);
			/**
			 * @brief Get a unique_id for a corresponding rule
			 * The id 0 is reserved for null
			 */
			CfgRule const &_get_rule(uint32_t id);
			TableState _get_var(std::string const &str);
			std::string _state_str(TableState const &state) const;

			/**
			 * @brief Finds the deepest rulest at each position
			 * Keeps track of the inbetween positions while drilling down
			 */
			void _drill(
				TableState const &start,
				TableState &children,
				bool is_root=true
			);

			/**
			 * @brief Wrapper around base _drill func
			 */
			TableState _drill(TableState const &start);

			/**
			 * @brief A helper function to group a set of rules by leaf.
			 * @param[in] The list of rules to group
			 */
			std::vector<RuleGroup> _group_rules(TableState const &children);
	};
}

namespace cg {
	using AbsoluteSolver = abs::AbsoluteSolver;
}

inline std::ostream &operator<<(std::ostream &os, cg::abs::StackElement const &element) {
	return element.debug(os);
}

