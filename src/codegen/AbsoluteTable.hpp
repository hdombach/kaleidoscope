# pragma once

#include <vector>

#include "util/IterAdapter.hpp"
#include "util/result.hpp"
#include "CfgContext.hpp"

namespace cg::abs {
	/**
	 * @brief A reference to a position inside a rule production
	 */
	class RulePos {
		public:
			/**
			 * @brief Creates an empty RulePos
			 */
			RulePos() = default;
			/**
			 * @brief Creates a RulePos with an index into a production rule
			 * @param[in] set The rule set index in the context
			 * @param[in] rule The rule index in the rule set
			 * @param[in] offset The leaf index in the rule
			 * @param[in] context The context the position is indexing into
			 */
			RulePos(
				uint32_t set,
				uint32_t rule,
				uint32_t offset,
				CfgContext const &context
			);

			/**
			 * @brief Gets the corresponding rule set in the provided CfgContext
			 */
			CfgRuleSet const &set() const;
			/**
			 * @brief Gets the corresponding rule in the provided CfgContext
			 */
			CfgRule const &rule() const;
			/**
			 * @brief Gets the corresponding leaf in the provided CfgContext
			 */
			CfgLeaf const &leaf() const;

			/**
			 * @brief Gets the next leaf for the corresponding rule.
			 */
			RulePos next_leaf() const;

			/**
			 * @brief The string representation
			 */
			std::string str() const;

			/**
			 * @brief Is the position referencing the end of the rule
			 * If yes, this RulePos does not have a valid leaf associated with it
			 */
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
	 * @brief The state of a table row consisting of a group of rules positions
	 */
	class TableState {
		public:
			using Container = std::set<RulePos>;
			using iterator = Container::iterator;
			using const_iterator = Container::const_iterator;

			/**
			 * @brief Creates an empty TableState
			 */
			TableState() = default;

			iterator begin();
			iterator end();
			const_iterator begin() const;
			const_iterator end() const;

			/**
			 * @brief Adds a new rule
			 */
			void add_rule(RulePos const &pos);
		
			/**
			 * @brief Does the TableState contain a RulePos
			 */
			bool contains(RulePos const &pos) const;

			/**
			 * @brief Is the TableState empty
			 */
			bool empty() const;

			/**
			 * @brief A string representation of a TableState
			 */
			std::string str() const;

			/**
			 * @brief Does the TableState contain a position at the end of a rule
			 */
			bool contains_end() const;

			/**
			 * @brief Finds a rule containing the end position
			 * Will throw an error if the number of end rules does not equal 1
			 */
			util::Result<RulePos, void> find_end() const;

			/**
			 * @brief Creates a new TableState with every RulePos incrimented by one
			 */
			TableState step();

			bool operator==(TableState const &other) const;
		private:
			Container _rules;
	};

	/**
	 * @brief Table for quickly parsing a file
	 */
	class AbsoluteTable {
		public:
			using Row = util::IterAdapter<uint32_t*>;
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
			 * @brief Returns a row for a corresponding table_state.
			 * If the row doesn't exist yet, create a new row
			 * @param[in] table_state
			 */
			Row row(TableState const &table_state);

			uint32_t row_id(TableState const &table_state);

			/**
			 * @brief Returns a row for a corresponding state id
			 * @param[in] table_state
			 */
			Row row(StateId const &state_id);

			/**
			 * @brief does a row exist for a table_state yet
			 * @param[in] table_state
			 * @returns bool
			 */
			bool contains_row(TableState const &table_state);

			/**
			 * @brief Looks up a cell for a corresponding row and character
			 * @param[in] row
			 * @param[in] c
			 * @returns id of the new state
			 */
			StateId &lookup_char(TableState const &row, char c);

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
			StateId &lookup_ruleset(TableState const &row, uint32_t ruleset);

			/**
			 * @brief Looks up a cell for a corresponding row and ruleset id
			 * @param[in] state_id
			 * @param[in] c
			 * @returns id of the new state
			 */
			StateId &lookup_ruleset(uint32_t state_id, uint32_t ruleset);

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
			std::vector<TableState> _table_states;
	};
}
