#include "AbsoluteSolver.hpp"

#include <algorithm>
#include <glm/detail/qualifier.hpp>
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

		auto initial = r._get_var(root);

		auto children = std::set<RulePos>();
		r._drill(initial, children);

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

	TEST(AbsoluteSolver, simplify_strings) {
		auto c = CfgContext();

		c.prim("opening") = c.s("< ");
		c.prim("closing") = c.s(" >");
		c.prim("message") = c["opening"] + c.s("Hello world") + c["closing"];

		c.prep();
		log_debug() << "before simplifying:\n" << c << std::endl;
		c.simplify();
		log_debug() << "after simplifying:\n" << c << std::endl;

		auto solver = AbsoluteSolver::setup(c, "message");
	}

	TEST(AbsoluteSolver, simplify_sets) {
		auto c = CfgContext();

		c.prim("digit-pair") = c.i("0123456789") + c.s(".") + c.i("1234567890");
		c.prim("str") = c.s("\"") + c.e("\"") + c.s("\"");

		c.prep();
		c.simplify();

		auto solver = AbsoluteSolver::setup(c, "digit-pair");
	}

	AbsoluteSolver::State AbsoluteSolver::_get_state(uint32_t index) {
		return util::Adapt(
			&_states[index * _state_size],
			&_states[index * (_state_size + 1)]
		);
	}

	uint32_t &AbsoluteSolver::_state_char(State state, char c) {
		return state.begin()[c];
	}

	uint32_t &AbsoluteSolver::_state_ruleset(State state, uint32_t s) {
		return state.begin()[128 + s];
	}

	uint32_t AbsoluteSolver::_add_state(StateRule const &state_rule) {
		{
			auto rule = std::find(_state_rules.begin(), _state_rules.end(), state_rule);
			if (rule != _state_rules.end()) {
				return rule - _state_rules.begin();
			}
		}

		auto r = _states.size() / _state_size;

		for (auto i = 0; i < _state_size; i++) {
			_states.push_back(0);
		}

		auto state = _get_state(r);

		auto unsorted_rules = _drill(state_rule);
		//Do not check a character. Instead reduce.
		//Right now, can only reduce if it is the only rule in a group
		if (_has_end_of_rule(unsorted_rules)) {
			if (unsorted_rules.size() != 1) {
				log_error() << "Can't generate without a lookahead" << std::endl;;
			}
			for (auto &s : state) {
				s = unsorted_rules.begin()->set | REDUCE_MASK;
			}
			return r;
		}

		//group by leaf type. We already handeled positions at the end a rule so we
		//don't need to handle that
		auto groups = _group_rules(unsorted_rules);
		for (auto &[leaf, rules] : groups) {
			log_assert(
				leaf.type() == CfgLeaf::Type::character || leaf.type() == CfgLeaf::Type::var,
				"CfgContext must be simplified before using AbsoluteSolver"
			);

			auto next = _step_state_rule(rules);
			auto next_i = _add_state(next);

			if (leaf.type() == CfgLeaf::Type::character) {
				_state_char(state, leaf.str_content()[0]) = next_i;
			} else {
				_state_ruleset(state, _ctx->rule_id(leaf.var_name())) = next_i;
			}
		}

		return r;
	}

	bool AbsoluteSolver::_has_end_of_rule(StateRule const &state) const {
		for (auto &pos : state) {
			if (pos.offset >= _get_rule(pos).leaves().size()) {
				return true;
			}
		}
		return false;
	}

	AbsoluteSolver::StateRule AbsoluteSolver::_step_state_rule(StateRule const &state) {
		auto r = StateRule();
		for (auto rule : r) {
			rule.offset++;
			r.insert(rule);
		}
		return r;
	}

	CfgRuleSet const &AbsoluteSolver::_get_set(RulePos const &pos) const {
		auto &sets = _ctx->cfg_rule_sets();

		log_assert(sets.size() > pos.set, "Invalid set position inside RulePos");

		return sets[pos.set];
	}
	CfgRule const &AbsoluteSolver::_get_rule(RulePos const &pos) const {
		auto &rules = _get_set(pos).rules();

		log_assert(rules.size() > pos.rule, "Invalid rule position inside RulePos");

		return rules[pos.rule];
	}
	CfgLeaf const &AbsoluteSolver::_get_leaf(RulePos const &pos) const {
		auto &leaves = _get_rule(pos).leaves();

		log_assert(leaves.size() > pos.offset, "Invalid leaf position inside RulePos");

		return leaves[pos.offset];
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

	std::string AbsoluteSolver::_debug(RulePos const &pos) const {
		auto ss = std::stringstream();

		auto set = _get_set(pos);
		auto rule = _get_rule(pos);

		ss << set.name() << " -> ";
		for (int i = 0; i < rule.leaves().size(); i++) {
			if (pos.offset == i) {
				ss << ". ";
			}
			ss << rule.leaves()[i] << " ";
		}
		if (pos.offset == rule.leaves().size()) {
			ss << " .";
		}

		return ss.str();
	}

	void AbsoluteSolver::_drill(
		std::set<RulePos> const &start,
		std::set<RulePos> &children,
		bool is_root
	) {
		for (auto &rule : start) {
			auto &leaf = _get_leaf(rule);
			if (leaf.type() == CfgLeaf::Type::var) {
				if (!is_root) {
					children.insert(rule);
				}
				auto all_children = _get_var(leaf.var_name());
				auto unique_children = std::set<RulePos>();
				std::set_difference(
					all_children.begin(), all_children.end(),
					children.begin(), children.end(),
					std::inserter(unique_children, unique_children.begin())
				);

				_drill(unique_children, children, false);
			} else {
				children.insert(rule);
			}
		}
	}

	std::set<RulePos> AbsoluteSolver::_drill(std::set<RulePos> const &start) {
		auto r = std::set<RulePos>();
		_drill(start, r);
		return r;
	}

	std::vector<AbsoluteSolver::RuleGroup> AbsoluteSolver::_group_rules(
		std::set<RulePos> const &children
	) {
		auto r = std::vector<AbsoluteSolver::RuleGroup>();
		for (auto &child : children) {
			auto &leaf = _get_leaf(child);
			RuleGroup *matched = nullptr;
			for (auto &group : r) {
				if (group.leaf == leaf) {
					matched = &group;
				}
			}
			if (matched == nullptr) {
				r.push_back(RuleGroup());
				matched = &r[r.size() - 1];
				matched->leaf = leaf;
			}
			log_assert(matched, "matched must be initialized.");
			matched->rules.insert(child);
		}
		return r;
	}
}
