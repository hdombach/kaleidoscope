#pragma once

#include "codegen/CfgContext.hpp"
#include "util/IterAdapter.hpp"
#include <vector>
#include <set>

namespace cg {
	struct RulePos {
		uint32_t set;
		uint32_t rule;
		uint32_t offset;

		bool operator<(RulePos const &rhs) const;
		bool operator==(RulePos const &rhs) const;
		bool operator>(RulePos const &rhs) const;
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
	 */
	class AbsoluteSolver {
		public:
			static util::Result<AbsoluteSolver, KError> setup(
				CfgContext const &context,
				std::string const &root
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
			std::vector<StateRule> _state_rules;
			std::vector<uint32_t> _states;
			/**
			 * @brief How many uints are used for a single state
			 */
			uint32_t _state_size;

		private:
			/**
			 * @param[in] Index of the state.
			 * @returns The state reference
			 */
			State _get_state(uint32_t index);
			/**
			 * Gets the next state for a given char
			 * @param[in] state current position in table
			 * @param[in] c Character to lookup in the state
			 * @returns Index of the state
			 */
			uint32_t _get_state_char(State state, char c);
			/**
			 * Gets the next state for a given rule set
			 */
			uint32_t _get_state_set(State state, uint32_t s);
			/**
			 * @brief Creates a state for a given rule
			 * @param[in] state_rule
			 * @returns Index in _states of created state
			 */
			uint32_t _add_state(StateRule const &state_rule);

			CfgRuleSet const &_get_set(RulePos const &pos) const;
			CfgRule const &_get_rule(RulePos const &pos) const;
			CfgLeaf const &_get_leaf(RulePos const &pos) const;
			std::set<RulePos> _get_var(std::string const &str);

			std::string _debug(RulePos const &pos) const;

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
