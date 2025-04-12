#include "CfgNode.hpp"

#include "util/Util.hpp"
#include "util/log.hpp"
#include <sstream>

namespace cg {
	CfgLeaf::CfgLeaf(): _type(Type::none) {}

	CfgLeaf CfgLeaf::str(std::string const &str) {
		return CfgLeaf(Type::str, str, true);
	}
	CfgLeaf CfgLeaf::include(std::string const &str) {
		return CfgLeaf(Type::set, str, true);
	}
	CfgLeaf CfgLeaf::exclude(std::string const &str) {
		return CfgLeaf(Type::set, str, false);
	}
	CfgLeaf CfgLeaf::var(std::string const &str) {
		return CfgLeaf(Type::var, str, true);
	}

	util::Result<uint32_t, void> CfgLeaf::match(std::string const &str) const {
		switch (_type) {
			case Type::str:
				for (int i = 0; i < _content.size(); i++) {
					if (str[i] != _content[i]) {
						return {};
					}
				}
				return _content.size();
			case Type::set:
				if (str[0] == '\0') return {};
				if ((std::find(_content.begin(), _content.end(), str[0]) != _content.end()) == _include) {
					return 1;
				} else {
					return {};
				}
			default:
				log_fatal_error() << "Unknown match in CfgLeaf" << std::endl;
				return {};
		}
	}

	std::ostream& CfgLeaf::print_debug(std::ostream &os) const {
		switch (_type) {
			case Type::str:
				os << "\"" << util::escape_str(_content) << "\"";
				break;
			case Type::var:
				os << "<" << _content << ">";
				break;
			case Type::set:
				if (!_include) {
					os << "!";
				}
				os << "[" << util::escape_str(_content) << "]";
				break;
			case Type::none:
				os << "<unknown>";
		}
		return os;
	}

	std::string CfgLeaf::str() const {
		auto ss = std::stringstream();
		print_debug(ss);
		return ss.str();
	}

	bool CfgLeaf::operator==(CfgLeaf const &other) const {
		return _type == other._type &&
			_content == other._content &&
			_include == other._include;
	}

	bool CfgLeaf::operator!=(CfgLeaf const &other) const {
		return _type != other._type ||
			_content != other._content ||
			_include != other._include;
	}

	CfgLeaf::CfgLeaf(Type type, std::string const &str, bool include):
		_type(type),
		_content(str),
		_include(include)
	{}

	CfgRule::CfgRule(CfgLeaf const &leaf): _leaves{leaf} { }

	CfgRule::CfgRule(CfgRule const &lhs, CfgRule const &rhs) {
		_leaves = lhs._leaves;
		for (auto &leaf : rhs._leaves) {
			_leaves.push_back(leaf);
		}
	}

	void CfgRule::seperate_leaves() {
		auto new_leaves = std::vector<CfgLeaf>();
		for (auto &leaf : _leaves) {
			if (leaf.type() == CfgLeaf::Type::str) {
				for (auto c : leaf.str_content()) {
					new_leaves.push_back(CfgLeaf::include({c}));
				}
			} else {
				new_leaves.push_back(leaf);
			}
		}
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

	CfgRuleSet::CfgRuleSet(std::string const &name): _name(name) { }

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

	std::ostream& CfgRuleSet::print_debug(std::ostream &os) const {
		bool is_first = true;
		for (auto &rule : _rules) {
			if (is_first) {
				is_first = false;
			} else {
				os << " | ";
			}
			os << rule;
		}
		return os;
	}

	std::string CfgRuleSet::str() const {
		auto ss = std::stringstream();
		print_debug(ss);
		return ss.str();
	}
}
