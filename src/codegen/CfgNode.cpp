#include "CfgNode.hpp"

#include "util/Util.hpp"
#include "util/log.hpp"
#include <sstream>

namespace cg {
	CfgLeaf::CfgLeaf(): _type(Type::empty) {}

	CfgLeaf::CfgLeaf(TType type): _type(Type::token), _token_type(type) {}

	CfgLeaf CfgLeaf::var(std::string const &str) {
		auto l = CfgLeaf();
		l._type = Type::var;
		l._var_name = str;
		return l;
	}

	std::ostream& CfgLeaf::print_debug(std::ostream &os) const {
		switch (_type) {
			case Type::empty:
				return os << "Empty";
			case Type::token:
				return os << _token_type;
			case Type::var:
				return os << "<" << _var_name << ">";
		}
	}

	std::string CfgLeaf::str() const {
		auto ss = std::stringstream();
		print_debug(ss);
		return ss.str();
	}

	bool CfgLeaf::operator==(CfgLeaf const &other) const {
		if (_type != other._type) return false;
		switch (_type) {
			case Type::empty:
				return true;
			case Type::token:
				return _token_type == other._token_type;
			case Type::var:
				return _var_name == other._var_name;
		}
	}

	bool CfgLeaf::operator!=(CfgLeaf const &other) const {
		return !(*this == other);
	}

	CfgRule::CfgRule(CfgLeaf const &leaf): _leaves{leaf} { }

	CfgRule::CfgRule(CfgRule const &lhs, CfgRule const &rhs) {
		_leaves = lhs._leaves;
		for (auto &leaf : rhs._leaves) {
			_leaves.push_back(leaf);
		}
	}

	CfgRule::CfgRule(std::vector<CfgLeaf> const &leaves): _leaves(leaves) {}

	uint32_t CfgRule::set_id() const {
		return _set_id;
	}

	void CfgRule::set_set_id(uint32_t id) {
		_set_id = id;
	}

	std::ostream& CfgRule::print_debug(std::ostream &os) const {
		bool is_first = true;
		for (auto &leaf : _leaves) {
			if (is_first) {
				is_first = false;
			} else {
				os << " ";
			}
			os << leaf;
		}
		return os;
	}

	std::string CfgRule::str() const {
		auto ss = std::stringstream();
		print_debug(ss);
		return ss.str();
	}

	bool CfgRule::operator==(CfgRule const &other)const{
		return _leaves == other._leaves;
	}

	bool CfgRule::operator!=(CfgRule const &other)const{
		return _leaves != other._leaves;
	}

	CfgRuleSet::CfgRuleSet(std::string const &name): _name(name) { }

	CfgRuleSet::CfgRuleSet(std::string const &name, std::vector<CfgRule> &&rules):
		_name(name),
		_rules(rules)
	{}

	CfgRuleSet& CfgRuleSet::operator=(CfgRuleSet const &set) {
		add_rules(set);
		return *this;
	}

	CfgRuleSet& CfgRuleSet::operator=(CfgRule const &rule) {
		add_rule(rule);
		return *this;
	}

	CfgRuleSet& CfgRuleSet::operator=(CfgLeaf const &leaf) {
		add_rule(leaf);
		return *this;
	}

	void CfgRuleSet::add_rule(CfgRule const &rule) {
		_rules.push_back(rule);
	}

	void CfgRuleSet::add_rules(CfgRuleSet const &set) {
		for (auto &rule : set.rules()) {
			add_rule(rule);
		}
	}

	CfgRuleSet::iterator CfgRuleSet::begin() {
		return _rules.begin();
	}

	CfgRuleSet::const_iterator CfgRuleSet::begin() const {
		return _rules.begin();
	}

	CfgRuleSet::iterator CfgRuleSet::end() {
		return _rules.end();
	}

	CfgRuleSet::const_iterator CfgRuleSet::end() const {
		return _rules.end();
	}

	std::ostream& CfgRuleSet::print_debug(std::ostream &os, bool multiline) const {
		bool is_first = true;
		os << name() << " -> ";
		for (auto &rule : _rules) {
			if (is_first) {
				is_first = false;
			} else {
				if (multiline) {
					os << "\n  ";
				}
				os << " | ";
			}
			os << rule;
		}
		return os;
	}

	std::string CfgRuleSet::str(bool multiline) const {
		auto ss = std::stringstream();
		print_debug(ss, multiline);
		return ss.str();
	}
}
