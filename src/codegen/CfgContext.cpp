#include "CfgContext.hpp"
#include "tests/Test.hpp"
#include "util/KError.hpp"
#include "util/log.hpp"


namespace cg {
	CfgContext::Ptr CfgContext::create() {
		return std::make_unique<CfgContext>();
	}
	CfgLeaf CfgContext::t(Token::Type t) const {
		return CfgLeaf(t);
	}
	CfgLeaf CfgContext::empty() const {
		return CfgLeaf();
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

	void CfgContext::debug_print(std::ostream &os) const {
		for (auto &set : _cfg_rule_sets) {
			os << set.str(true) << std::endl;
		}
	}

	util::Result<void, KError> CfgContext::prep() { 
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
					} else if (leaf.type() == CfgLeaf::Type::empty) {
						if (rule.leaves().size() != 1) {
							return KError::codegen(util::f(
								"The empty leaf must be in it's own rule:\n",
								set.str(true)
							));
						}
					}
				}
			}
		}

		//Make sure rule is the correct format
		{
			auto root = *get_root();
			if (root.rules().size() != 1) {
				return KError::codegen(util::f("Root rule set must consist of one rule."));
			}

			auto rule = root.rules().front();
			auto msg = "Rot rule must have the structure \"<var_name> eof\"";

			if (rule.leaves().size() != 2) {
				return KError::codegen(msg);
			}
			if (rule.leaves().front().type() != CfgLeaf::Type::var) {
				return KError::codegen(msg);
			}
			if (rule.leaves().back().type() != CfgLeaf::Type::token) {
				return KError::codegen(msg);
			}
			if (rule.leaves().back().token_type() != Token::Type::Eof) {
				return KError::codegen(msg);
			}
		}

		_update_set_ids();

		return {};
	}

	void CfgContext::simplify() {
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
						if (leaf.type() == CfgLeaf::Type::empty) {
							log_assert(rule.leaves().size() == 1, "Empty must be in it's own rule");
							break;
						}
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
				log_assert(rule.leaves().size() > 0, "Should be impossible to have empty rule");
				if (rule.leaves().front().type() == CfgLeaf::Type::empty) continue;
				auto stack = std::vector<CfgLeaf>();
				_enumerate_empty(empty_sets, rule, 0, stack, new_rules);
			}
			if (new_rules.size() > 0) {
				new_sets.push_back(CfgRuleSet(set.name(), std::move(new_rules)));
			}
		}
		_cfg_rule_sets = std::move(new_sets);
		_update_set_ids();
	}

	void CfgContext::_update_set_ids() {

		for (uint32_t i = 0; i < _cfg_rule_sets.size(); i++) {
			auto &set = _cfg_rule_sets[i];
			for (auto &rule : set.rules()) {
				rule.set_set_id(i);
			}
		}
	}
}
