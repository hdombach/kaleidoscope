#include "AbsoluteSolver.hpp"

#include <glm/detail/qualifier.hpp>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "util/KError.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"

namespace cg::abs {
	util::Result<AbsoluteSolver, KError> AbsoluteSolver::setup(
		const CfgContext &context,
		const std::string &root
	) {
		auto r = AbsoluteSolver();
		r._ctx = &context;
		r._table = AbsoluteTable(context);

		auto initial = r._get_var(root);

		auto new_state = TableState();
		auto children = TableState();
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
		std::string const &filename
	) {
		try {
			uint32_t node_id = 0;
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
						another_c = '\x03';
					}
					action = _table.lookup_char(cur_state_id, another_c);
				}

				log_debug() << "action: " << _table.action_str(action) << std::endl;

				if (action == AbsoluteTable::ACCEPT_ACTION) {
					log_debug() << "Accepting" << std::endl;
					break;
				} else if (action & AbsoluteTable::REDUCE_MASK) {
					uint32_t production_rule_id = action & ~AbsoluteTable::REDUCE_MASK;
					cur_state_id = _reduce(
						stack,
						production_rule_id,
						stack[stack.size() - 1].state_id(),
						node_id
					);
					prev_rule_set = _get_rule_set_id(production_rule_id);
				} else if (prev_rule_set == std::nullopt) {
					if (!*c) {
						return KError::codegen("Reached EOF unexpectedly");
					}
					//TODO: update source_location
					stack.push_back(StackElement(
						cur_state_id,
						AstNode::create_str(node_id++, {*c}, std::source_location())
					));
					log_debug() << "shifted character: " << *c << std::endl;
					cur_state_id = action;
					c++;
				} else {
					cur_state_id = action;
					prev_rule_set = std::nullopt;
				}
				log_debug() << "state is " << cur_state_id << std::endl;
			}
			if (stack.size() != 1) {
				return KError::codegen("Stack must contain 1 element");
			}
			if (!stack.back().is_node()) {
				return KError::codegen("Stack must contain a node");
			}
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
		uint32_t cur_state_id,
		uint32_t &node_id
	){
		auto ss = std::stringstream();
		ss << "Reducing stack: " << util::plist(stack) << " -> ";

		auto &rule = _get_rule(rule_id);
		log_debug() << "Reducing rule " << rule << std::endl;
		log_assert(rule.leaves().size() <= stack.size(), "Stack must contain enough elements for the rule");
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
				log_debug() << "popped size is: " << popped.size() << std::endl;
				log_assert(p.leaf().type() == CfgLeaf::Type::character, "CfgLeaf must be a character");
				node.add_child(AstNode::create_str(node_id++, p.leaf().str(), TODO2));
			}
		}
		stack.push_back(StackElement(cur_state_id, node));
		ss << util::plist(stack) << std::endl;
		log_debug() << ss.str();
		log_debug() << "popped size is " << popped.size() << std::endl;
		return popped.back().state_id();
	}

	uint32_t AbsoluteSolver::_add_state(TableState const &state_rule) {
		log_debug() << "Adding state: " << _table.state_str(state_rule) << std::endl;
		if (_table.contains_row(state_rule)) return _table.row_id(state_rule);

		auto new_state = _table.row(state_rule);
		auto new_state_id = _table.row_id(state_rule);

		auto unsorted_rules = _drill(state_rule);

		//Do not check a character. Instead reduce.
		//Right now, can only reduce if it is the only rule in a group
		if (auto end_rule = _get_end_rule(unsorted_rules)) {
			for (auto &s : new_state) {
				s = _get_rule_id(end_rule.value()) | AbsoluteTable::REDUCE_MASK;
			}
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

			auto next = rules.step();

			log_debug() << "Stepped step with leaf:" << leaf << std::endl << _state_str(next);

			if (leaf.type() == CfgLeaf::Type::character) {
				if (leaf.str_content()[0] == '\x03') {
					_table.lookup_char(state_rule, leaf.str_content()[0]) = AbsoluteTable::ACCEPT_ACTION;
				} else {
					_table.lookup_char(state_rule, leaf.str_content()[0]) = _add_state(next);
				}
			} else {
				_table.lookup_ruleset(state_rule, _ctx->rule_id(leaf.var_name())) = _add_state(next);
			}
		}

		return new_state_id;
	}

	util::Result<RulePos, void> AbsoluteSolver::_get_end_rule(
		TableState const &state
	) const {
		auto r = util::Result<RulePos, void>();
		for (auto &pos : state) {
			if (pos.is_end()) {
				if (r) {
					log_error() << "There is more than one end rule" << std::endl;
				}
				r = pos;
			}
		}
		return r;
	}

	uint32_t AbsoluteSolver::_get_rule_id(RulePos const &pos) {
		uint32_t r = 1;
		auto &target = pos.rule();
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

	TableState AbsoluteSolver::_get_var(std::string const &str) {
		auto r = TableState();
		for (uint32_t i = 0; i < _ctx->cfg_rule_sets().size(); i++) {
			auto &set = _ctx->cfg_rule_sets()[i];
			if (set.name() == str) {
				for (uint32_t j = 0; j < set.rules().size(); j++) {
					r.add_rule(RulePos(i, j, 0, *_ctx));
				}
				return r;
			}
		}

		log_error() << "Couldn't find var of name " << str << std::endl;
		return {};
	}

	std::string AbsoluteSolver::_state_str(TableState const &state) const {
		auto r = std::string();
		for (auto &rule : state) {
			r += rule.str() + "\n";
		}
		return r;
	}

	void AbsoluteSolver::_drill(
		TableState const &start,
		TableState &children,
		bool is_root
	) {
		for (auto &rule : start) {
			if (rule.is_end()) {
				children.add_rule(rule);
				continue;
			}
			auto &leaf = rule.leaf();
			if (leaf.type() == CfgLeaf::Type::var) {
				if (!is_root) {
					children.add_rule(rule);
				}
				auto all_children = _get_var(leaf.var_name());
				auto unique_children = TableState();
				for (auto &pos : all_children) {
					if (!children.contains(pos)) {
						unique_children.add_rule(pos);
					}
				}

				_drill(unique_children, children, false);
			} else {
				children.add_rule(rule);
			}
		}
	}

	TableState AbsoluteSolver::_drill(TableState const &start) {
		auto r = TableState();
		_drill(start, r, false);
		return r;
	}

	std::vector<AbsoluteSolver::RuleGroup> AbsoluteSolver::_group_rules(
		TableState const &children
	) {
		auto r = std::vector<AbsoluteSolver::RuleGroup>();
		if (children.empty()) return r;
		for (auto &child : children) {
			if (child.is_end()) continue;
			auto &leaf = child.leaf();
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
			matched->rules.add_rule(child);
		}
		return r;
	}

	std::ostream &operator<<(std::ostream &os, AbsoluteSolver::StackElement const &e) {
		return e.debug(os);
	}
}
