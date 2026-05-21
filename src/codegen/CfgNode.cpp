#include "CfgNode.hpp"

#include "util/Util.hpp"
#include "util/log.hpp"
#include <sstream>

namespace cg {
	CfgLeaf::CfgLeaf(): _type(Type::empty) {}

	CfgLeaf::CfgLeaf(int type): _type(Type::token), _token_type(type) {}

	CfgLeaf CfgLeaf::var(std::string const &str) {
		auto l = CfgLeaf();
		l._type = Type::var;
		l._var_name = str;
		return l;
	}

	CfgLeaf CfgLeaf::cls(CfgRuleSet const &rule_set) {
		auto l = CfgLeaf();
		l._type = Type::cls;
		l._rule_set = SetPtr(new CfgRuleSet(rule_set));
		return l;
	}

	CfgLeaf CfgLeaf::cls(CfgRule const &rule) {
		auto l = CfgLeaf();

		auto rs = new CfgRuleSet();
		rs->add_rule(rule);

		l._type = Type::cls;
		l._rule_set = SetPtr(rs);
		return l;
	}

	CfgLeaf::CfgLeaf(CfgLeaf const &other) {
		_type = other._type;
		_token_type = other._token_type;
		_var_name = other._var_name;

		if (_type == Type::cls) {
			_rule_set = SetPtr(new CfgRuleSet(other.rule_set()));
		}
	}

	CfgLeaf::CfgLeaf(CfgLeaf &&other) {
		_type = other._type;
		_token_type = other._token_type;
		_var_name = std::move(other._var_name);
		_rule_set = std::move(other._rule_set);
	}

	CfgLeaf &CfgLeaf::operator=(CfgLeaf const &other) {
		_type = other._type;
		_token_type = other._token_type;
		_var_name = other._var_name;

		if (_type == Type::cls) {
			_rule_set = SetPtr(new CfgRuleSet(other.rule_set()));
		}

		return *this;
	}

	CfgLeaf &CfgLeaf::operator=(CfgLeaf &&other) {
		_type = other._type;
		_token_type = other._token_type;
		_var_name = std::move(other._var_name);
		_rule_set = std::move(other._rule_set);

		return *this;
	}

	CfgLeaf::Type CfgLeaf::type() const {
		return _type;
	}

	int CfgLeaf::token_type() const {
		return _token_type;
	}

	std::string const &CfgLeaf::var_name() const {
		log_assert(_type == Type::var, "Must test type before getting var_name");
		return _var_name;
	}

	CfgRuleSet &CfgLeaf::rule_set() {
		log_assert(_rule_set.get(), "Must test type before getting rule set");
		return *_rule_set;
	}

	CfgRuleSet const &CfgLeaf::rule_set() const {
		log_assert(_rule_set.get(), "Must test type before getting rule set");
		return *_rule_set;
	}

	std::ostream& CfgLeaf::print_debug(
		std::ostream &os,
		Token::Config const &tok_config
	) const {
		switch (_type) {
			case Type::empty:
				return os << "Empty";
			case Type::token:
				return os << tok_config.name_table[_token_type];
			case Type::var:
				return os << "<" << _var_name << ">";
			case Type::cls:
				os << "[";
				_rule_set->print_debug(os, tok_config);
				os << "]";
				return os;
		}
	}

	std::string CfgLeaf::str(Token::Config const &tok_config) const {
		auto ss = std::stringstream();
		print_debug(ss, tok_config);
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
			case Type::cls:
				return rule_set() == other.rule_set();
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

	std::vector<CfgLeaf> const &CfgRule::leaves() const { return _leaves; }

	std::vector<CfgLeaf> &CfgRule::leaves() { return _leaves; }

	uint32_t CfgRule::set_id() const {
		return _set_id;
	}

	void CfgRule::set_set_id(uint32_t id) {
		_set_id = id;
	}

	std::ostream& CfgRule::print_debug(
		std::ostream &os,
		Token::Config const &tok_config
	) const {
		bool is_first = true;
		for (auto &leaf : _leaves) {
			if (is_first) {
				is_first = false;
			} else {
				os << " ";
			}
			leaf.print_debug(os, tok_config);
		}
		return os;
	}

	std::string CfgRule::str(Token::Config const &tok_config) const {
		auto ss = std::stringstream();
		print_debug(ss, tok_config);
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

	CfgRuleSet& CfgRuleSet::operator=(int const &leaf_type) {
		add_rule(CfgRule(leaf_type));
		return *this;
	}

	bool CfgRuleSet::operator==(CfgRuleSet const &other) const {
		return _name == other._name && _rules == other._rules;
	}

	bool CfgRuleSet::operator!=(CfgRuleSet const &other) const {
		return *this != other;
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

	std::ostream& CfgRuleSet::print_debug(
		std::ostream &os,
		Token::Config const &tok_config,
		bool multiline
	) const {
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
			rule.print_debug(os, tok_config);
		}
		return os;
	}

	std::string CfgRuleSet::str(
		Token::Config const &tok_config,
		bool multiline
	) const {
		auto ss = std::stringstream();
		print_debug(ss, tok_config, multiline);
		return ss.str();
	}

	CfgClosure::CfgClosure(CfgRuleSet const &rule_set): _rule_set(rule_set) {}

	CfgRuleSet const &CfgClosure::rule_set() const { return _rule_set; }
}
