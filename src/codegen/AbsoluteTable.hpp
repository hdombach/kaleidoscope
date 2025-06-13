# pragma once

#include <vector>

#include "util/IterAdapter.hpp"
#include "util/result.hpp"
#include "CfgContext.hpp"

namespace cg::abs {
	class RulePos {
		public:
			RulePos() = default;
			RulePos(
				uint32_t set,
				uint32_t rule,
				uint32_t offset,
				CfgContext const &context
			);

			CfgRuleSet const &set() const;
			CfgRule const &rule() const;
			CfgLeaf const &leaf() const;

			RulePos next_leaf();

			std::string str() const;

			bool is_end() const;

			bool operator<(RulePos const &rhs) const;
			bool operator==(RulePos const &rhs) const;
			bool operator>(RulePos const &rhs) const;

		private:
			uint32_t _set=0;
			uint32_t _rule=0;
			uint32_t _offset=0;
			CfgContext const *_ctx=nullptr;
	};

	/**
	 * @brief Contains a table 
	 */
	class AbsoluteTable {
		public:
			using StateRow = util::IterAdapter<uint32_t*>;
			using StateRule = std::set<RulePos>;
			using StateId = uint32_t;

			static const uint32_t ACCEPT_ACTION = 0x88888888;
			static const uint32_t REDUCE_MASK = 0x80000000;

			AbsoluteTable() = default;
			AbsoluteTable(CfgContext const &ctx);

			/**
			 * @brief Prints a debug table
			 * @param[in] os: ostream to print to
			 * @param[in] chars: what characters to include in the table
			 */
			void print(std::ostream &os, std::set<char> const &chars);

			/**
			 * @brief Returns a row for a corresponding state_rule.
			 * If the row doesn't exist yet, create a new row
			 * @param[in] state_rule
			 */
			StateRow row(StateRule const &state_rule);

			uint32_t row_id(StateRule const &state_rule);

			/**
			 * @brief Returns a row for a corresponding state id
			 * @param[in] state_rule
			 */
			StateRow row(StateId const &state_id);

			/**
			 * @brief does a row exist for a state_rule yet
			 * @param[in] state_rule
			 * @returns bool
			 */
			bool contains_row(StateRule const &state_rule);

			/**
			 * @brief Looks up a cell for a corresponding row and character
			 * @param[in] row
			 * @param[in] c
			 * @returns id of the new state
			 */
			StateId &lookup_char(StateRule const &row, char c);

			/**
			 * @brief Looks up a cell for a corresponding row and character
			 * @param[in] state_id
			 * @param[in] c
			 * @returns id of the new state
			 */
			StateId &lookup_char(uint32_t state_id, char c);

			/**
			 * @brief Gets the next state for a given set
			 * @param[in] index of row
			 */
			StateId &lookup_ruleset(StateRule const &row, uint32_t ruleset);

			/**
			 * @brief Looks up a cell for a corresponding row and ruleset id
			 * @param[in] state_id
			 * @param[in] c
			 * @returns id of the new state
			 */
			StateId &lookup_ruleset(uint32_t state_id, uint32_t ruleset);

			/**
			 * @brief Gets the string representatin of a group of states
			 * @param[in] state
			 * @returns String representatin
			 */
			std::string state_str(StateRule const &state) const;

			/**
			 * @brief Gets the string representation of an action
			 * @param[in] uint32_t action 
			 * @returns String representation
			 */
			std::string action_str(uint32_t action) const;


		private:
			CfgContext const *_ctx = nullptr;
			uint32_t _state_size;
			std::vector<uint32_t> _states;
			std::vector<StateRule> _state_rules;
	};
}
