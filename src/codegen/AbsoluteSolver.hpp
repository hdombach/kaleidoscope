#pragma once

#include "codegen/CfgContext.hpp"
#include "util/IterAdapter.hpp"
#include <vector>

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

		private:
			using State = util::IterAdapter<uint32_t*>;
			using StateRule = std::vector<RulePos>;

		private:
			CfgContext const *_ctx = nullptr;
			std::vector<StateRule> _state_rules;
			std::vector<uint32_t> _states;
			/**
			 * @brief How many uints are used for a single state
			 */
			uint32_t _state_size;

		private:
			State _get_state(uint32_t index);
			uint32_t _get_state_char(State state, char c);
			/**
			 * @brief Creates a state for a given rule
			 * @param[in] state_rule
			 * @returns Index in _states of created state
			 */
			uint32_t _add_state(StateRule const &state_rule);

			CfgLeaf const &_get(RulePos const &pos);
			std::set<RulePos> _get_var(std::string const &str);

			/**
			 * @brief Finds the deepest rulest at each position
			 * Keeps track of the inbetween positions while drilling down
			 */
			void _drill(
				std::set<RulePos> const &start,
				std::set<RulePos> &terminals,
				std::set<RulePos> &nonterminals,
				bool is_root=true
			);
	};
}
