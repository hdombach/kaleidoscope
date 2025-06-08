#include "AbsoluteSolver.hpp"

#include <algorithm>
#include <glm/detail/qualifier.hpp>
#include <iterator>
#include <string>
#include <variant>
#include <vector>

#include "util/KError.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"

namespace cg {
	int node_id = 0;
	util::Result<AbsoluteSolver, KError> AbsoluteSolver::setup(
		const CfgContext &context,
		const std::string &root
	) {
		auto r = AbsoluteSolver();
		r._ctx = &context;
		r._table = AbsoluteTable(context);

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
		return _table.print(os, chars);
	}

	util::Result<AstNode, KError> AbsoluteSolver::parse(
		std::string const &str,
		std::string const &root_name,
		std::string const &filename
	) {
		try {
			// uint32_t is the current state
			auto stack = std::vector<StackElement>();
			uint32_t cur_state_id=0;
			auto c = str.data();
			auto prev_rule_set = std::optional<uint32_t>();
			while (1) {
				uint32_t action;
				if (prev_rule_set) {
					action = _table.lookup_ruleset(cur_state_id, *prev_rule_set);
				} else {
					auto another_c = *c;
					if (another_c == 0) {
						another_c = '\x7f';
					}
					action = _table.lookup_char(cur_state_id, another_c);
				}

				log_debug() << "action: " << _table.action_str(action) << std::endl;

				if (action & AbsoluteTable::REDUCE_MASK) {
					uint32_t production_rule_id = action & ~AbsoluteTable::REDUCE_MASK;
					cur_state_id = _reduce(stack, production_rule_id, stack[stack.size() - 1].state_id());
					prev_rule_set = _get_rule_set_id(production_rule_id);
				} else if (prev_rule_set == std::nullopt) {
					if (*c == '\0') break;
					//TODO: update source_location
					stack.push_back(StackElement(cur_state_id, AstNode::create_str(node_id++, {*c}, std::source_location())));
					log_debug() << "shifted character: " << *c << std::endl;
					cur_state_id = action;
					c++;
				} else {
					cur_state_id = action;
					prev_rule_set = std::nullopt;
				}
				log_debug() << "state is " << cur_state_id << std::endl;
			}
			log_assert(stack.size() == 1, "Stack must contain 1 element");
			log_assert(stack.back().is_node(), "Stack must contain a node");
			return stack.back().node();
		} catch_kerror;
	}

	AbsoluteSolver::StackElement::StackElement():
	_state_id(0),
	_value(CfgLeaf())
	{}

	AbsoluteSolver::StackElement::StackElement(
		uint32_t state_id,
		CfgLeaf const &leaf
	) {
		_state_id = state_id;
		_value = leaf;
	}

	AbsoluteSolver::StackElement::StackElement(
		uint32_t state_id,
		AstNode const &node
	) {
		_state_id = state_id;
		_value = node;
	}

	uint32_t AbsoluteSolver::StackElement::state_id() const {
		return _state_id;
	}

	CfgLeaf const &AbsoluteSolver::StackElement::leaf() const {
		return std::get<CfgLeaf>(_value);
	}

	CfgLeaf &AbsoluteSolver::StackElement::leaf() {
		return std::get<CfgLeaf>(_value);
	}

	bool AbsoluteSolver::StackElement::is_leaf() const {
		return std::holds_alternative<CfgLeaf>(_value);
	}

	AstNode const &AbsoluteSolver::StackElement::node() const {
		return std::get<AstNode>(_value);
	}

	AstNode &AbsoluteSolver::StackElement::node(){
		return std::get<AstNode>(_value);
	}

	bool AbsoluteSolver::StackElement::is_node() const {
		return std::holds_alternative<AstNode>(_value);
	}

	std::ostream &AbsoluteSolver::StackElement::debug(std::ostream &os) const {
		os << "(" << _state_id << ") ";
		if (is_node()){
			os << node();
		} else {
			os << leaf();
		}
		return os;
	}

	uint32_t AbsoluteSolver::_reduce(
		std::vector<StackElement> &stack,
		uint32_t rule_id,
		uint32_t cur_state_id
	){
		auto &debug = log_debug();
		debug << "Reducing stack: " << util::plist(stack) << " -> ";

		auto &rule = _get_rule(rule_id);
		auto popped = std::vector<StackElement>();
		for (uint32_t i = 0; i < rule.leaves().size(); i++) {
			popped.push_back(stack.back());
			stack.pop_back();
		}
		auto TODO2 = util::FileLocation();
		auto node = AstNode::create_rule(node_id++, rule.str(), TODO2);

		for (int i = popped.size()-1; i >= 0; i--){
			auto &p = popped[i];
			if (p.is_node()) {
				node.add_child(p.node());
			} else {
				log_assert(p.leaf().type() == CfgLeaf::Type::character, "CfgLeaf must be a character");
				node.add_child(AstNode::create_str(node_id++, p.leaf().str(), TODO2));
			}
		}
		stack.push_back(StackElement(cur_state_id, node));
		debug << util::plist(stack) << std::endl;
		return popped.back().state_id();
	}

	uint32_t AbsoluteSolver::_add_state(StateRule const &state_rule) {
		log_debug() << "Adding state: " << _table.state_str(state_rule) << std::endl;
		if (_table.contains_row(state_rule)) return _table.row_id(state_rule);

		auto new_state = _table.row(state_rule);
		auto new_state_id = _table.row_id(state_rule);

		auto unsorted_rules = _drill(state_rule);

		//Do not check a character. Instead reduce.
		//Right now, can only reduce if it is the only rule in a group
		if (_has_end_of_rule(unsorted_rules)) {
			if (unsorted_rules.size() != 1) {
				log_error() << "Can't generate without a lookahead" << std::endl;;
			}
			for (auto &s : new_state) {
				s = _get_rule_id(*unsorted_rules.begin()) | AbsoluteTable::REDUCE_MASK;
			}
			return new_state_id;
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

			if (leaf.type() == CfgLeaf::Type::character) {
				_table.lookup_char(state_rule, leaf.str_content()[0]) = next_i;
			} else {
				_table.lookup_ruleset(state_rule, _ctx->rule_id(leaf.var_name())) = next_i;
			}
		}

		return new_state_id;
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

	uint32_t AbsoluteSolver::_get_rule_id(RulePos const &pos) {
		uint32_t r = 1;
		auto &target = _get_rule(pos);
		for (auto &set : _ctx->cfg_rule_sets()) {
			for (auto &rule : set.rules()) {
				if (rule == target) return r;
				r++;
			}
		}

		log_fatal_error() << "No matching rule position in the contex" << std::endl;
		return r;
	}

	CfgRule const &AbsoluteSolver::_get_rule(uint32_t id) {
		log_assert(id != 0, "Can't get rule for id 0");
		uint32_t r = 1;
		for (auto &set : _ctx->cfg_rule_sets()) {
			for (auto &rule : set.rules()) {
				if (r == id) return rule;
				r++;
			}
		}

		log_fatal_error() << "No matching rule id in contex" << std::endl;
		return _ctx->cfg_rule_sets()[0].rules()[0];
	}

	CfgRule const &AbsoluteSolver::_get_rule(RulePos const &pos) const {
		auto &rules = _get_set(pos).rules();

		log_assert(rules.size() > pos.rule, "Invalid rule position inside RulePos");

		return rules[pos.rule];
	}

	uint32_t AbsoluteSolver::_get_rule_set_id(uint32_t rule_id) const {
		log_assert(rule_id != 0, "Can't get rule for id 0");
		uint32_t r = 1;
		uint32_t set_id = 0;
		for (auto &set : _ctx->cfg_rule_sets()) {
			for (auto &rule : set.rules()) {
				if (r == rule_id) return set_id;
				r++;
			}
			set_id++;
		}
		log_fatal_error() << "No matching rule id in context" << std::endl;
		return 0;
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

	std::ostream &operator<<(std::ostream &os, AbsoluteSolver::StackElement const &e) {
		return e.debug(os);
	}
}
