#include "AbsoluteSolver.hpp"

#include <algorithm>
#include <glm/detail/qualifier.hpp>
#include <iterator>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "util/IterAdapter.hpp"
#include "util/KError.hpp"
#include "util/PrintTools.hpp"
#include "util/log.hpp"

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
		r._add_state(r._get_var(root));

		return r;
	}

	void AbsoluteSolver::print_table(
		std::ostream &os,
		std::set<char> const &chars
	) {
		auto table = std::vector<std::vector<std::string>>();
		auto label_row = std::vector<std::string>();
		label_row.push_back("state");
		label_row.push_back("current rules");
		for (auto c : chars) {
			label_row.push_back(util::f("\"", c, "\""));
		}
		for (auto &rule : _ctx->cfg_rule_sets()) {
			label_row.push_back(rule.name());
		}
		table.push_back(label_row);

		for (int rule_i = 0; rule_i < _state_rules.size(); rule_i++) {
			auto row = std::vector<std::string>();
			row.push_back(std::to_string(rule_i));
			row.push_back(_state_str(_state_rules[rule_i]));
			auto state  = _get_state(rule_i);
			uint32_t state_i = 0;
			for (auto s : state) {
				auto state_str = std::string();
				if (s & REDUCE_MASK) {
					state_str += "r";
				}
				state_str += std::to_string(s & ~REDUCE_MASK);

				if (state_i < 128) {
					if (chars.contains(state_i)) {
						row.push_back(state_str);
					}
				} else {
					row.push_back(state_str);
				}
				state_i++;
			}
			table.push_back(row);
		}

		os << util::ptable(table);
	}

	util::Result<AstNode, KError> AbsoluteSolver::parse(
		std::string const &str,
		std::string const &root_name,
		std::string const &filename
	) {
		try {
			log_assert(!_states.empty(), "Must load CFG into AbsoluteSolver");

			// uint32_t is the current state
			using Element = std::variant<CfgLeaf, AstNode, uint32_t>;
			auto stack = std::vector<Element>();
			auto c = str.begin();
			while (c != str.end()) {
				auto &last = stack[stack.size() - 1];
				log_assert(
					std::holds_alternative<uint32_t>(last),
					"Top of the stack must be a state"
				);
				auto cur_state = _get_state(std::get<uint32_t>(last));
				auto cell = _state_char(cur_state, *c);
				if (cell & REDUCE_MASK) {

				}
			}

			log_assert(stack.size() == 1, "stack must contain 1 element");
			log_assert(std::holds_alternative<AstNode>(stack[0]), "Element in stack must be an AstNode");
			return std::get<AstNode>(stack[0]);
		} catch_kerror;
	}

	AbsoluteSolver::State AbsoluteSolver::_get_state(uint32_t index) {
		return util::Adapt(
			&_states[index * _state_size],
			&_states[(index + 1) * _state_size]
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

		_state_rules.push_back(state_rule);

		const auto r = _states.size() / _state_size;

		for (auto i = 0; i < _state_size; i++) {
			_states.push_back(0);
		}

		auto state = _get_state(r);

		log_debug() << "drilling: " << _state_str(state_rule) << std::endl;
		auto unsorted_rules = _drill(state_rule);
		log_debug() << "drilled: " << _state_str(unsorted_rules) << std::endl;

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

			log_debug() << "Stepping step:" << std::endl << _state_str(rules);

			auto next = _step_state_rule(rules);

			log_debug() << "Stepped step with leaf:" << leaf << std::endl << _state_str(next);

			auto next_i = _add_state(next);
			state = _get_state(r); // pointers are potentially incorrectly now

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
			if (_is_end_pos(pos)) {
				return true;
			}
		}
		return false;
	}

	bool AbsoluteSolver::_is_end_pos(RulePos const &pos) const {
		return pos.offset >= _get_rule(pos).leaves().size();
	}

	AbsoluteSolver::StateRule AbsoluteSolver::_step_state_rule(StateRule const &state) {
		auto r = StateRule();
		for (auto rule : state) {
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

	std::string AbsoluteSolver::_rule_str(RulePos const &pos) const {
		auto r = std::string();
		auto &rule = _get_rule(pos);
		int i = 0;
		for (auto &leaf : rule.leaves()) {
			if (i == pos.offset) {
				r += "|";
			}
			r += leaf.str();
			i++;
		}
		if (pos.offset >= rule.leaves().size()) {
			r += "|";
		}
		return r;
	}

	std::string AbsoluteSolver::_state_str(StateRule const &state) const {
		auto r = std::string();
		for (auto &rule : state) {
			r += _rule_str(rule) + "\n";
		}
		return r;
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
			if (_is_end_pos(rule)) {
				children.insert(rule);
				continue;
			}
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
		_drill(start, r, false);
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
