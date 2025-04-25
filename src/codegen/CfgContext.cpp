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

	CfgRuleSet const *CfgContext::get(std::string const &name) const {
		if (_cfg_map.contains(name)) {
			return &_cfg_rule_sets[_cfg_map.at(name)];
		} else {
			return nullptr;
		}
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
