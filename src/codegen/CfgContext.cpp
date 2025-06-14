#include "CfgContext.hpp"
#include "util/log.hpp"

#include <sstream>

namespace cg {
	CfgLeaf CfgContext::s(std::string const &str) const {
		return CfgLeaf::str(str);
	}
	CfgLeaf CfgContext::i(std::string const &str) const {
		return CfgLeaf::include(str);
	}
	CfgLeaf CfgContext::e(std::string const &str) const {
		return CfgLeaf::exclude(str);
	}
	CfgLeaf CfgContext::c(char c) const {
		return CfgLeaf::character(c);
	}
	CfgLeaf CfgContext::eof() const {
		return CfgLeaf::character('\x03');
	}
	CfgLeaf CfgContext::operator[](std::string const &str) const {
		return CfgLeaf::var(str);
	}

	CfgRuleSet &CfgContext::prim(std::string const &name) {
		_prim_names.insert(name);
		if (!_cfg_map.contains(name)) {
			_cfg_rule_sets.push_back(CfgRuleSet(name));
			_cfg_map.emplace(name, _cfg_rule_sets.size() - 1);
		}
		return _cfg_rule_sets[_cfg_map[name]];
	}
	CfgRuleSet &CfgContext::temp(std::string const &name) {
		if (!_cfg_map.contains(name)) {
			_cfg_rule_sets.push_back(CfgRuleSet(name));
			_cfg_map.emplace(name, _cfg_rule_sets.size() - 1);
		}
		return _cfg_rule_sets[_cfg_map[name]];
	}
	CfgRuleSet &CfgContext::root(std::string const &name) {
		if (_root_name.empty()) {
			log_assert(!_cfg_map.contains(name), "Cannot create root if rule already exists");
			_root_name = name;
			_cfg_rule_sets.push_back(CfgRuleSet(_root_name));
			_cfg_map.emplace(name, _cfg_rule_sets.size() - 1);
		} else {
			log_error() << "Cannot create root " << name
				<< " since root " << name << " already exists" << std::endl;
		}
		return _cfg_rule_sets[_cfg_map[_root_name]];
	}

	CfgRuleSet const *CfgContext::get(std::string const &name) const {
		if (_cfg_map.contains(name)) {
			return &_cfg_rule_sets[_cfg_map.at(name)];
		} else {
			return nullptr;
		}
	}

	CfgRuleSet const *CfgContext::get_root() const {
		return get(_root_name);
	}

	std::vector<CfgRuleSet> const &CfgContext::cfg_rule_sets() const {
		return _cfg_rule_sets;
	}

	uint32_t CfgContext::rule_id(std::string const &name) const {
		return _cfg_map.at(name);
	}

	void CfgContext::debug_set(
		CfgRuleSet const &set,
		std::ostream &os
	) const {
		os << set.name() << " -> " << set;
	}

	void CfgContext::debug_set(
		std::string const &set,
		std::ostream &os
	) const {
		if (auto node = get(set)) {
			debug_set(*node, os);
		} else {
			os << "<anon node>";
		}
	}

	void CfgContext::debug_sets(std::ostream &os) const {
		for (auto &set : _cfg_rule_sets) {
			debug_set(set, os);
			os << std::endl;
		}
	}

	std::string CfgContext::set_str(CfgRuleSet const &set) const {
		auto ss = std::stringstream();

		debug_set(set, ss);

		return ss.str();
	}

	std::string CfgContext::set_str(std::string const &name) const {
		auto ss = std::stringstream();

		debug_set(name, ss);

		return ss.str();
	}

	util::Result<void, KError> CfgContext::prep() { 
		log_debug() << "name is \"" << _root_name << "\"" << _root_name.empty() << std::endl;
		if (_root_name.empty()) {
			log_debug() << "root name is empty" << std::endl;
			return KError::codegen("CfgContext must contain a root");
		}
		for (auto &[key, i] : _cfg_map) {
			auto &set = _cfg_rule_sets[i];
			for (auto &rule : set.rules()) {
				for (auto &leaf : rule.leaves()) {
					if (leaf.type() == CfgLeaf::Type::var) {
						if (!_cfg_map.contains(leaf.var_name())) {
							return KError::codegen(util::f(
								"Variable name ",
								leaf.var_name(),
								" does not exist."
							));
						}
					}
				}
			}
		}
		return {};
	}

	void CfgContext::simplify() {
		for (auto &set : _cfg_rule_sets) {
			for (auto &rule : set.rules()) {
				rule.seperate_leaves();
			}
		}

		for (auto &set : _cfg_rule_sets) {
			set.simplify_char_sets();
		}
	}
}
