#include "AbsoluteTable.hpp"
#include "util/PrintTools.hpp"
#include "util/IterAdapter.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"

namespace cg::abs {
	RulePos::RulePos(
		uint32_t set,
		uint32_t rule,
		uint32_t offset,
		CfgContext const &context
	):
		_set(set),
		_rule(rule),
		_offset(offset),
		_ctx(&context)
	{}

	CfgRuleSet const &RulePos::set() const {
		log_assert(_ctx, "CfgContext must be provided");
		auto &sets = _ctx->cfg_rule_sets();

		log_assert(sets.size() > _set, "Invalid set position inside RulePos");

		return sets[_set];
	}

	uint32_t RulePos::set_index() const {
		return _set;
	}

	CfgRule const &RulePos::rule() const {
		auto &rules = set().rules();
		log_assert(rules.size() > _rule, "Invalid rule position inside RulePos");
		return rules[_rule];
	}

	uint32_t RulePos::rule_index() const {
		return _rule;
	}

	CfgLeaf const &RulePos::leaf() const {
		auto &leaves = rule().leaves();

		log_assert(leaves.size() > _offset, "Invalid leaf position inside RulePos");

		return leaves[_offset];
	}

	uint32_t RulePos::leaf_index() const {
		return _offset;
	}

	RulePos RulePos::next_leaf() const {
		auto r = *this;
		r._offset++;
		log_assert(r._offset <= rule().leaves().size(), "Can't step beyond the end of the rule");
		return r;
	}

	std::string RulePos::str() const {
		auto str = std::string();
		auto &r = rule();
		int i = 0;
		for (auto &leaf : r.leaves()) {
			if (i == _offset) {
				str += "|";
			}
			str += leaf.str();
			i++;
		}
		if (_offset >= r.leaves().size()) {
			str += "|";
		}
		return str;
	}

	bool RulePos::is_end() const {
		return _offset >= rule().leaves().size();
	}

	bool RulePos::operator<(RulePos const &rhs) const {
		if (_set < rhs._set) {
			return true;
		} else if (_set > rhs._set) {
			return false;
		}
		if (_rule < rhs._rule) {
			return true;
		} else if (_rule > rhs._rule) {
			return false;
		}
		if (_offset < rhs._offset) {
			return true;
		} else {
			return false;
		}
	}

	bool RulePos::operator==(RulePos const &rhs) const {
		return _set == rhs._set && _rule == rhs._rule && _offset == rhs._offset;
	}

	bool RulePos::operator>(RulePos const &rhs) const {
		if (_set > rhs._set) {
			return true;
		} else if (_set < rhs._set) {
			return false;
		}
		if (_rule > rhs._rule) {
			return true;
		} else if (_rule < rhs._rule) {
			return false;
		}
		if (_offset > rhs._offset) {
			return true;
		} else {
			return false;
		}
	}

	TableState::iterator TableState::begin() {
		return _rules.begin();
	}

	TableState::iterator TableState::end() {
		return _rules.end();
	}

	TableState::const_iterator TableState::begin() const {
		return _rules.begin();
	}
	TableState::const_iterator TableState::end() const {
		return _rules.end();
	}

	void TableState::add_rule(RulePos const &pos) {
		_rules.insert(pos);
	}

	bool TableState::contains(RulePos const &pos) const {
		return _rules.count(pos) > 0;
	}

	bool TableState::empty() const {
		return _rules.empty();
	}

	std::string TableState::str() const {
		auto r = std::string();
		for (auto &rule : _rules) {
			r += rule.str() + "\n";
		}
		return r;
	}

	bool TableState::contains_end() const {
		for (auto &pos : _rules) {
			if (pos.is_end()) {
				return true;
			}
		}
		return false;
	}

	util::Result<RulePos, void> TableState::find_end() const {
		auto result = util::Result<RulePos, void>();

		for (auto &rule : _rules) {
			if (rule.is_end()) {
				if (result.has_value()) {
					return {};
				} else {
					result = rule;
				}
			}
		}

		return result;
	}

	TableState TableState::step() {
		auto r = TableState();
		for (auto rule : _rules) {
			r.add_rule(rule.next_leaf());
		}
		return r;
	}

	bool TableState::operator==(TableState const &other) const {
		return _rules == other._rules;
	}

	AbsoluteTable::AbsoluteTable(CfgContext const &cfg) {
		_ctx = &cfg;
		_state_size = 128 + _ctx->cfg_rule_sets().size();
	}

	void AbsoluteTable::print(std::ostream &os, std::set<char> const &chars) {
		log_assert(_ctx, "CfgContext must be provided");
		auto table = std::vector<std::vector<std::string>>();
		auto label_row = std::vector<std::string>();
		label_row.push_back("state");
		label_row.push_back("current rules");
		for (auto c : chars) {
			label_row.push_back(util::f("\"", util::escape_str({c}), "\""));
		}
		for (auto &rule : _ctx->cfg_rule_sets()) {
			label_row.push_back(rule.name());
		}
		table.push_back(label_row);


		for (int i = 0; i < _table_states.size(); i++) {
			auto row_str = std::vector<std::string>();
			row_str.push_back(std::to_string(i));
			row_str.push_back(_table_states[i].str());
			auto state  = row(_table_states[i]);
			uint32_t state_i = 0;
			for (auto s : state) {
				auto state_str = action_str(s);

				if (state_i < 128) {
					if (chars.contains(state_i)) {
						row_str.push_back(state_str);
					}
				} else {
					row_str.push_back(state_str);
				}
				state_i++;
			}
			table.push_back(row_str);
		}

		os << util::ptable(table);
	}

	AbsoluteTable::Row AbsoluteTable::row(TableState const &table_state) {
		auto r = std::find(
			_table_states.begin(),
			_table_states.end(),
			table_state
		);
		size_t index = r - _table_states.begin();
		//State rule does not already exist. Create a new one
		if (r == _table_states.end()) {
			_table_states.push_back(table_state);
			for (auto i = 0; i < _state_size; i++) {
				_states.push_back(0);
			}
		}
		return row(index);
	}

	uint32_t AbsoluteTable::row_id(TableState const &table_state) {
		auto rule = std::find(_table_states.begin(), _table_states.end(), table_state);
		return rule - _table_states.begin();
	}

	AbsoluteTable::Row AbsoluteTable::row(StateId const &state_id) {
		return util::Adapt(
			&_states[state_id * _state_size],
			&_states[(state_id + 1) * _state_size]
		);
	}

	bool AbsoluteTable::contains_row(TableState const &table_state) {
		return util::contains(_table_states, table_state);
	}

	AbsoluteTable::StateId &AbsoluteTable::lookup_char(
		TableState const &r,
		char c
	) {
		return row(r).begin()[c];
	}

	AbsoluteTable::StateId &AbsoluteTable::lookup_char(
		uint32_t state_id,
		char c
	) {
		return row(state_id).begin()[c];
	}

	AbsoluteTable::StateId &AbsoluteTable::lookup_ruleset(
		TableState const &r,
		uint32_t ruleset
	) {
		return row(r).begin()[ruleset + 128];
	}

	AbsoluteTable::StateId &AbsoluteTable::lookup_ruleset(
		uint32_t state_id,
		uint32_t ruleset
	) {
		return row(state_id).begin()[ruleset + 128];
	}

	std::string AbsoluteTable::action_str(uint32_t action) const {
		auto s = std::string();
		if (action == ACCEPT_ACTION) {
			return "accept";
		}
		if (action & REDUCE_MASK) {
			s += "r";
		}
		s += std::to_string(action & ~REDUCE_MASK);
		return s;
	}
}
