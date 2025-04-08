#include "AbsoluteSolver.hpp"

#include <algorithm>
#include <iterator>

#include "util/IterAdapter.hpp"
#include "util/log.hpp"
#include "tests/Test.hpp"

namespace cg {
	bool RulePos::operator<(RulePos const &rhs) const {
		if (set < rhs.set) {
			return true;
		} else if (set > rhs.set) {
			return false;
		}
		if (rule < rhs.rule) {
			return true;
		} else if (rule > rhs.rule) {
			return false;
		}
		if (offset < rhs.offset) {
			return true;
		} else {
			return false;
		}
	}

	bool RulePos::operator==(RulePos const &rhs) const {
		return set == rhs.set && rule == rhs.rule && offset == rhs.offset;
	}

	bool RulePos::operator>(RulePos const &rhs) const {
		if (set > rhs.set) {
			return true;
		} else if (set < rhs.set) {
			return false;
		}
		if (rule > rhs.rule) {
			return true;
		} else if (rule < rhs.rule) {
			return false;
		}
		if (offset > rhs.offset) {
			return true;
		} else {
			return false;
		}
	}

	util::Result<AbsoluteSolver, KError> AbsoluteSolver::setup(
		const CfgContext &context,
		const std::string &root
	) {
		auto r = AbsoluteSolver();
		r._ctx = &context;
		r._state_size = 128 + context.cfg_rule_sets().size();

		log_debug() << "Creating AbsoluteSolver" << std::endl;
		auto initial = r._get_var(root);
		for (auto &pos : initial) {
			log_debug() << "Starting with rule: " << r._get(pos) << std::endl;
		}

		log_debug() << "Drilling down" << std::endl;

		auto terminals = std::set<RulePos>();
		auto nonterminals = std::set<RulePos>();
		r._drill(initial, terminals, nonterminals);

		for (auto &t : nonterminals) {
			log_debug() << "Got nonterminal: " << r._get(t) << std::endl;
		}
		for (auto &term : terminals) {
			log_debug() << "Got terminal: " << r._get(term) << std::endl;
		}

		return r;
	}

	TEST(AbsoluteSolver, debug_print) {
		auto c = CfgContext();
		c.prim("S") = c["E"];
		c.prim("E") = c["E"] + c.s("*") + c["B"];
		c.prim("E") = c["E"] + c.s("+") + c["B"];
		c.prim("E") = c["B"];
		c.prim("B") = c.s("0");
		c.prim("B") = c.s("1");

		auto solver = AbsoluteSolver::setup(c, "S");
	}

	AbsoluteSolver::State AbsoluteSolver::_get_state(uint32_t index) {
		return util::Adapt(
			&_states[index * _state_size],
			&_states[index * (_state_size + 1)]
		);
	}

	uint32_t AbsoluteSolver::_get_state_char(State state, char c) {
		return state.begin()[c];
	}

	uint32_t AbsoluteSolver::_add_state(AbsoluteSolver::StateRule const &state_rule) {
		auto r = _states.size();

		for (auto i = 0; i < _state_size; i++) {
			_states.push_back(0);
		}

		for (auto &pos : state_rule) {
		}

		return r;
	}

	CfgLeaf const &AbsoluteSolver::_get(RulePos const &pos) {
		return _ctx->cfg_rule_sets()[pos.set].rules()[pos.rule].leaves()[pos.offset];
	}

	std::set<RulePos> AbsoluteSolver::_get_var(std::string const &str) {
		auto r = std::set<RulePos>();
		for (uint32_t i = 0; i < _ctx->cfg_rule_sets().size(); i++) {
			auto &set = _ctx->cfg_rule_sets()[i];
			if (set.name() == str) {
				for (uint32_t j = 0; j < set.rules().size(); j++) {
					r.insert({i, j, 0});
				}
				return r;
			}
		}

		log_error() << "Couldn't find var of name " << str << std::endl;
		return {};
	}

	void AbsoluteSolver::_drill(
		std::set<RulePos> const &start,
		std::set<RulePos> &terminals,
		std::set<RulePos> &nonterminals,
		bool is_root
	) {
		for (auto &rule : start) {
			auto &leaf = _get(rule);
			if (leaf.type() == CfgLeaf::Type::var) {
				if (!is_root) {
					nonterminals.insert(rule);
				}
				auto all_children = _get_var(leaf.var_name());
				auto unique_children = std::set<RulePos>();
				std::set_difference(
					all_children.begin(), all_children.end(),
					nonterminals.begin(), nonterminals.end(),
					std::inserter(unique_children, unique_children.begin())
				);

				_drill(unique_children, terminals, nonterminals, false);
			} else {
				terminals.insert(rule);
			}
		}
	}
}
