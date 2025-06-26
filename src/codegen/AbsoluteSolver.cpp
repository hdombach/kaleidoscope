#include "AbsoluteSolver.hpp"

#include <glm/detail/qualifier.hpp>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
#include <memory>

#include "util/KError.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"

namespace cg::abs {
	StackElement::StackElement() = default;

	StackElement::StackElement(AstNode const &node):
		_value(node)
	{}

	StackElement::StackElement(uint32_t table_state):
		_value(table_state)
	{}

	bool StackElement::is_table_state() const {
		return std::holds_alternative<uint32_t>(_value);
	}

	uint32_t StackElement::table_state() const {
		return std::get<uint32_t>(_value);
	}

	bool StackElement::is_node() const {
		return std::holds_alternative<AstNode>(_value);
	}

	AstNode const &StackElement::node() const {
		return std::get<AstNode>(_value);
	}

	std::ostream &StackElement::debug(std::ostream &os) const {
		if (is_table_state()) {
			return os << table_state();
		} else if (is_node()) {
			return os << node();
		} else {
			return os << "UNKNOWN";
		}
	}

	util::Result<AbsoluteSolver::Ptr, KError> AbsoluteSolver::create(
		CfgContext::Ptr &&context
	) {
		auto r = std::make_unique<AbsoluteSolver>();
		r->_ctx = std::move(context);

		for (auto &set : r->_ctx->cfg_rule_sets()) {
			for (auto &rule : set.rules()) {
				r->_rules.push_back(rule);
			}
		}

		r->_table = AbsoluteTable(*r->_ctx);

		auto initial = r->_get_var(r->_ctx->get_root()->name());

		//auto new_state = TableState();
		//auto children = TableState();
		//r->_drill(initial, children);
		r->_add_state(initial);

		return r;
	}

	void AbsoluteSolver::print_table(
		std::ostream &os,
		std::set<char> const &chars
	) {
		auto rule_strs = std::vector<std::string>();
		for (auto &set : _ctx->cfg_rule_sets()) {
			for (auto &rule : set.rules()) {
				rule_strs.push_back("<" + set.name() + "> -> " + rule.str());
			}
		}
		os << util::plist_enumerated(rule_strs, true);

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
			stack.push_back(StackElement(0));
			auto c = str.data();
			while (1) {
				log_assert(stack.back().is_table_state(), "Stack must end with a state");
				uint32_t cur_state_id = stack.back().table_state();
				char cur_c = *c;
				if (cur_c == 0) {
					cur_c = '\x03';
				}
				log_trace() << "state is " << cur_state_id << std::endl;
				log_trace() << "Looking up \"" << util::escape_str({cur_c}) << "\"" << std::endl;
				uint32_t action = _table.lookup_char(cur_state_id, cur_c);
				log_trace() << "action: " << _table.action_str(action) << std::endl;

				if (action == AbsoluteTable::ACCEPT_ACTION) {
					log_trace() << "Accepting" << std::endl;
					break;
				} else if (action & AbsoluteTable::REDUCE_MASK) {
					uint32_t production_rule_id = action & ~AbsoluteTable::REDUCE_MASK;
					_reduce(
						stack,
						production_rule_id,
						node_id
					);
				} else {
					if (!*c) {
						return KError::codegen("Reached EOF unexpectedly");
					}
					//TODO: update source_location
					stack.push_back(StackElement(
						AstNode::create_str(node_id++, {c, filename.c_str()})
					));
					log_trace() << "shifted character: " << *c << std::endl;
					stack.push_back(StackElement(action)); // push back the next state
					c++;
				}
			}
			if (stack.size() != 3) {
				log_debug() << "stack is " << util::plist(stack) << std::endl;
				return KError::codegen("Stack must contain 3 elements");
			}
			if (!stack[1].is_node()) {
				return KError::codegen("The second element must be a node");
			}
			return stack[1].node();
		} catch_kerror;
	}

	util::Result<AstNode, KError> AbsoluteSolver::parse(
		std::vector<Token> const &tokens
	) {
		return KError::internal("unimplimented");
	}

	CfgContext const &AbsoluteSolver::cfg() const {
		return *_ctx;
	}

	CfgContext &AbsoluteSolver::cfg() {
		return *_ctx;
	}

	void AbsoluteSolver::_reduce(
		std::vector<StackElement> &stack,
		uint32_t rule_id,
		uint32_t &node_id
	){
		auto ss = std::stringstream();
		ss << "Reducing stack: " << util::plist(stack) << " -> ";

		auto &rule = _get_rule(rule_id);
		log_trace() << "Reducing rule " << rule_id << std::endl;
		log_assert(rule.leaves().size() <= stack.size() * 2 + 1, "Stack must contain enough elements for the rule");
		auto popped = std::vector<StackElement>();
		for (uint32_t i = 0; i < rule.leaves().size(); i++) {
			log_assert(stack.back().is_table_state(), "Back of the stack must be a table state");
			stack.pop_back();
			log_assert(stack.back().is_node(), "Back of the satck must be a node");
			popped.push_back(stack.back());
			stack.pop_back();
		}
		auto new_node = AstNode::create_rule(node_id++, rule.str());

		for (int i = popped.size()-1; i >= 0; i--){
			new_node.add_child(popped[i].node());
		}

		auto cur_state_id = stack.back().table_state();
		auto cur_rule_set = _get_rule_set_id(rule_id);
		auto next_state_id = _table.lookup_ruleset(cur_state_id, cur_rule_set);

		log_trace() << "state is " << cur_state_id << std::endl;
		log_trace() << "Looking up <" << _ctx->cfg_rule_sets()[cur_rule_set].name() << ">" << std::endl;

		stack.push_back(StackElement(new_node));
		stack.push_back(StackElement(next_state_id));

		ss << util::plist(stack) << std::endl;
		log_trace() << ss.str();
	}

	uint32_t AbsoluteSolver::_add_state(TableState const &state_rule) {
		log_debug() << "Adding state: " << util::trim(state_rule.str()) << std::endl;
		if (_table.contains_row(state_rule)) return _table.row_id(state_rule);

		auto new_state = _table.row(state_rule);
		auto new_state_id = _table.row_id(state_rule);

		auto unsorted_rules = _drill(state_rule);

		//Do not check a character. Instead reduce.
		//Right now, can only reduce if it is the only rule in a group
		auto end_rules = unsorted_rules.find_ends();
		if (end_rules.size() == 1) {
			log_debug() << "Adding reduction for " << end_rules.front().str() << std::endl;
			for (auto &s : new_state) {
				s = _get_rule_id(end_rules.front()) | AbsoluteTable::REDUCE_MASK;
			}
		} else if (end_rules.size() > 1) {
			log_fatal_error() << "Multiple end rules for state: " << state_rule << std::endl;
		}

		//group by leaf type. We already handeled positions at the end a rule so we
		//don't need to handle that
		auto groups = _group_rules(unsorted_rules);
		for (auto &[leaf, rules] : groups) {
			/*
			log_assert(
				leaf.type() == CfgLeaf::Type::character || leaf.type() == CfgLeaf::Type::var,
				"CfgContext must be simplified before using AbsoluteSolver"
			);

			auto next = rules.step();

			if (leaf.type() == CfgLeaf::Type::character) {
				if (leaf.str_content()[0] == '\x03') {
					_table.lookup_char(state_rule, leaf.str_content()[0]) = AbsoluteTable::ACCEPT_ACTION;
				} else {
					_table.lookup_char(state_rule, leaf.str_content()[0]) = _add_state(next);
				}
			} else {
				_table.lookup_ruleset(state_rule, _ctx->rule_id(leaf.var_name())) = _add_state(next);
			}
			*/
		}

		return new_state_id;
	}

	uint32_t AbsoluteSolver::_get_rule_id(RulePos const &pos) {
		// One indexed to keep things consistent with other references in this file
		uint32_t result = 1;
		uint32_t set_i=0;
		for (auto &set : _ctx->cfg_rule_sets()) {
			if (pos.set_index() > set_i) {
				set_i++;
				result += set.rules().size();
			} else {
				result += pos.rule_index();
				break;
			}
		}
		log_assert(_rules[result-1] == pos.rule(), "_rules does not match layout of cfg_rule_sets");
		return result;
	}

	CfgRule const &AbsoluteSolver::_get_rule(uint32_t id) {
		log_assert(id != 0, "Can't get rule for id 0");
		return _rules[id-1];
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
}
