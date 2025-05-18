#pragma once

#include <ostream>
#include <vector>
#include <set>

#include "codegen/CfgContext.hpp"
#include "util/IterAdapter.hpp"
#include "util/result.hpp"
#include "AstNode.hpp"
#include "AbsoluteTable.hpp"

namespace cg {
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
	 */
	class AbsoluteSolver {
		public:
			static util::Result<AbsoluteSolver, KError> setup(
				CfgContext const &context,
				std::string const &root
			);

			void print_table(std::ostream &os, std::set<char> const &chars);

			util::Result<size_t, KError> match(
				std::string const &str,
				std::string const &root_node
			);

			util::Result<AstNode, KError> parse(
				std::string const &str,
				std::string const &root_node,
				std::string const &filename = "codegen"
			);

		public:
			using State = util::IterAdapter<uint32_t*>;
			using StateRule = std::set<RulePos>;

			struct RuleGroup {
				CfgLeaf leaf;
				StateRule rules;
			};


		private:
			CfgContext const *_ctx = nullptr;
			AbsoluteTable _table;

		private:

			/**
			 * @brief Creates a state for a given rule
			 * @param[in] state_rule
			 * @returns Index in _states of created state
			 */
			uint32_t _add_state(StateRule const &state_rule);

			/**
			 * Check if at least 
			 * Checks if a pos is at the end of a rule
			 */
			bool _has_end_of_rule(StateRule const &state) const;
			bool _is_end_pos(RulePos const &pos) const;

			/**
			 * Gets next position for every position inside the state rule
			 * @param[in] state The state to step
			 * @returns The stepped state
			 */
			StateRule _step_state_rule(StateRule const &state);

			CfgRuleSet const &_get_set(RulePos const &pos) const;
			CfgRule const &_get_rule(RulePos const &pos) const;
			CfgLeaf const &_get_leaf(RulePos const &pos) const;
			std::set<RulePos> _get_var(std::string const &str);
			std::string _rule_str(RulePos const &pos) const;
			std::string _state_str(StateRule const &state) const;

			/**
			 * @brief Finds the deepest rulest at each position
			 * Keeps track of the inbetween positions while drilling down
			 */
			void _drill(
				std::set<RulePos> const &start,
				std::set<RulePos> &children,
				bool is_root=true
			);

			/**
			 * @brief Wrapper around base _drill func
			 */
			std::set<RulePos> _drill(std::set<RulePos> const &start);

			/**
			 * @brief A helper function to group a set of rules by leaf.
			 * @param[in] The list of rules to group
			 */
			std::vector<RuleGroup> _group_rules(std::set<RulePos> const &children);
	};
}
