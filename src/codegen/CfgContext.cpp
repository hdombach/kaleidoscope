#include "CfgContext.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"

#include <sstream>

namespace cg {
	CfgContext::Ptr CfgContext::create() {
		return std::make_unique<CfgContext>();
	}
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

		log_debug() << "Before removing empty sets" << std::endl;
		log_debug() << *this;
		_remove_empty();
		log_debug() << "After removing empty sets" << std::endl;
		log_debug() << *this;
	}

	std::set<std::string> get_empty_sets(CfgContext &ctx) {
		auto sets = std::set<std::string>();

		//for (auto &set : ctx.cfg_rule_sets()) {
		//	for (auto &rule : set.rules()) {
		//		if (rule.leaves().empty()) sets.insert(set.name());
		//	}
		//}

		bool propigating = true;
		while (propigating) {
			propigating = false;
			for (auto &set : ctx.cfg_rule_sets()) {
				for (auto &rule : set.rules()) {
					bool is_empty = true;
					for (auto &leaf : rule.leaves()) {
						if (leaf.type() == CfgLeaf::Type::var) {
							if (!sets.contains(leaf.var_name())) {
								is_empty = false;
							}
						} else {
							is_empty = false;
						}
					}
					if (is_empty) {
						if (!sets.contains(set.name())) {
							sets.insert(set.name());
							propigating = true;
						}
					}
				}
			}
		}

		return sets;
	}

	void _enumerate_empty(
		std::set<std::string> const &empty_sets,
		CfgRule const &rule,
		uint32_t leaf_index,
		std::vector<CfgLeaf> &stack,
		std::vector<CfgRule> &result
	) {
		if (leaf_index == rule.leaves().size()) {
			if (stack.size() > 0) {
				result.push_back({stack});
			}
		} else {
			auto &leaf = rule.leaves()[leaf_index];
			stack.push_back(leaf);
			_enumerate_empty(empty_sets, rule, leaf_index+1, stack, result);
			stack.pop_back();
			if (leaf.type() == CfgLeaf::Type::var && empty_sets.contains(leaf.var_name())) {
				_enumerate_empty(empty_sets, rule, leaf_index+1, stack, result);
			}
		}
	}

	void CfgContext::_remove_empty() {
		auto empty_sets = get_empty_sets(*this);
		auto new_sets = std::vector<CfgRuleSet>();
		for (auto &set : _cfg_rule_sets) {
			auto new_rules = std::vector<CfgRule>();
			for (auto &rule : set.rules()) {
				if (rule.leaves().empty()) continue;
				auto stack = std::vector<CfgLeaf>();
				_enumerate_empty(empty_sets, rule, 0, stack, new_rules);
			}
			if (new_rules.size() > 0) {
				new_sets.push_back(CfgRuleSet(set.name(), std::move(new_rules)));
			}
		}
		_cfg_rule_sets = std::move(new_sets);
	}
}
