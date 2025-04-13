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

		log_debug() << "Creating AbsoluteSolver" << std::endl;
		auto initial = r._get_var(root);
		for (auto &pos : initial) {
			log_debug() << "Starting with rule: " << r._get_leaf(pos) << std::endl;
		}

		log_debug() << "Drilling down" << std::endl;

		auto children = std::set<RulePos>();
		r._drill(initial, children);

		for (auto &t : children) {
			log_debug() << "Got child: " << r._debug(t) << std::endl;
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
		log_debug() << "before simplifying:\n" << c << std::endl;
		c.simplify();
		log_debug() << "after simplifying:\n" << c << std::endl;

		auto solver = AbsoluteSolver::setup(c, "digit-pair");
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

		auto groups = _group_rules(_drill(state_rule));
		for (auto &group : groups) {
			//TODO: Add transitions to the state.
		}

		return r;
	}

	CfgRuleSet const &AbsoluteSolver::_get_set(RulePos const &pos) const {
		return _ctx->cfg_rule_sets()[pos.set];
	}
	CfgRule const &AbsoluteSolver::_get_rule(RulePos const &pos) const {
		return _get_set(pos).rules()[pos.rule];
	}
	CfgLeaf const &AbsoluteSolver::_get_leaf(RulePos const &pos) const {
		return _get_rule(pos).leaves()[pos.offset];
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
