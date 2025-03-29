#include "CfgContext.hpp"
#include "util/log.hpp"

#include <sstream>

namespace cg {
	CfgLeaf CfgContextTemp::s(std::string const &str) const {
		return CfgLeaf::str(str);
	}
	CfgLeaf CfgContextTemp::i(std::string const &str) const {
		return CfgLeaf::include(str);
	}
	CfgLeaf CfgContextTemp::e(std::string const &str) const {
		return CfgLeaf::exclude(str);
	}
	CfgLeaf CfgContextTemp::operator[](std::string const &str) const {
		return CfgLeaf::var(str);
	}

	CfgRuleSet &CfgContextTemp::prim(std::string const &name) {
		_prim_names.insert(name);
		return _cfg_map[name];
	}
	CfgRuleSet &CfgContextTemp::temp(std::string const &name) {
		return _cfg_map[name];
	}

	CfgRuleSet const *CfgContextTemp::get(std::string const &name) const {
		if (_cfg_map.contains(name)) {
			return &_cfg_map.at(name);
		} else {
			return nullptr;
		}
	}

	void CfgContextTemp::debug_set(
		CfgRuleSet const &set,
		std::ostream &os
	) const {
		for (auto &rule : set.rules()) {
			os << set.name() << " -> ";
		}
		return;
	}

	void CfgContextTemp::debug_set(
		std::string const &set,
		std::ostream &os
	) const {
		if (auto node = get(set)) {
			debug_set(*node, os);
		} else {
			os << "<anon node>";
		}
	}

	std::string CfgContextTemp::set_str(CfgRuleSet const &set) const {
		auto ss = std::stringstream();

		debug_set(set, ss);

		return ss.str();
	}

	std::string CfgContextTemp::set_str(std::string const &name) const {
		auto ss = std::stringstream();

		debug_set(name, ss);

		return ss.str();
	}

	util::Result<void, KError> CfgContextTemp::prep() {
		for (auto &[key, set] : _cfg_map) {
			for (auto &rule : set.rules()) {
				for (auto &leaf : rule.leaves()) {
					if (leaf.type() == CfgLeaf::Type::var) {
						if (!_cfg_map.contains(leaf.var_name())) {
							return KError::codegen(util::f(
								"Varialbe name ",
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

	CfgNode CfgContext::lit(std::string const &str) const {
		return CfgNode::literal(str);
	}

	CfgNode CfgContext::ref(std::string const &name) const {
		if (_cfg_map.count(name)) {
			return CfgNode::ref(_cfg_map.at(name));
		} else {
			return CfgNode::ref(name);
		}
	}

	CfgNode CfgContext::seq(CfgNode &&lhs, CfgNode &&rhs) const {
		return CfgNode::seq(std::move(lhs), std::move(rhs));
	}

	CfgNode CfgContext::alt(CfgNode &&lhs, CfgNode &&rhs) const {
		return CfgNode::alt(std::move(lhs), std::move(rhs));
	}

	CfgNode CfgContext::cls(CfgNode &&c) const {
		return CfgNode::cls(std::move(c));
	}

	CfgNode CfgContext::opt(CfgNode &&c) const {
		return CfgNode::opt(std::move(c));
	}

	CfgNode CfgContext::neg(CfgNode &&c) const {
		return CfgNode::neg(std::move(c));
	}

	CfgNode CfgContext::dup(std::string const &name) const {
		return _cfgs[_cfg_map.at(name)].dup();
	}

	util::Result<void, KError> _update_refs(std::map<std::string, uint32_t> &map, CfgNode &node) {
		if (node.type() == CfgNode::Type::reference) {
			if (node.ref_id() == 0) {
				if (map.count(node.ref_name()) == 0) {
					return KError::codegen("Internal: Unknown cfg name " + node.ref_name());
				}
				node.set_ref_id(map[node.ref_name()]);
			}
		}

		for (auto &child : node.children()) {
			_update_refs(map, child);
		}
		return {};
	}

	util::Result<void, KError> CfgContext::prep() {
		for (auto &[name, id] : _cfg_map) {
			auto &cfg = _cfgs[id];
			if (!cfg.has_name()) {
				cfg.add_name(id, name);
			}
		}
		for (auto &node : _cfgs) {
			auto res = _update_refs(_cfg_map, node);
			if (!res) return res.error();
		}
		return {};
	}

	bool CfgContext::contains(std::string const &name) const {
		return _cfg_map.count(name) != 0;
	}

	bool CfgContext::contains(uint32_t id) const {
		return _cfgs.contains(id);
	}

	CfgNode &CfgContext::prim(std::string const &name) {
		_prim_names.push_back(name);
		return temp(name);
	}

	CfgNode &CfgContext::temp(std::string const &name) {
		if (_cfg_map.count(name)) {
			log_warning() << "Trying to create CfgNode " << name << " which already exists" << std::endl;
			auto id = _cfg_map[name];
			return _cfgs[id];
		} else {
			auto id = _cfgs.get_id();
			_cfg_map[name] = id;

			auto node = CfgNode();
			node.add_name(id, name); /* Will soon be overritten by new value */
			_cfgs.insert(std::move(node));
			return _cfgs[id];
		}
	}

	CfgNode const *CfgContext::get(std::string const &name) const {
		if (_cfg_map.contains(name)) {
			return &_cfgs[_cfg_map.at(name)];
		} else  {
			return nullptr;
		}
	}

	CfgNode const &CfgContext::get(uint32_t id) const {
		return _cfgs[id];
	}

	void CfgContext::debug_node(CfgNode const &node, std::ostream &os) const {
		bool first = true;
		switch (node.type()) {
			case Type::none:
				return;
			case Type::literal:
				os << "\"" << node.content() << "\"";
				return;
			case Type::reference: {
					auto name = _cfgs[node.ref_id()].name();
					if (name.empty()) {
						os << "<anon>";
					} else {
						os << "<" << name << ">";
					}
					return;
				}
			case Type::sequence:
				for (auto &child : node.children()) {
					if (first) {
						first = false;
					} else {
						os << " + ";
					}

					if (child.type() == Type::alternative) {
						os << "(";
					}
					debug_node(child, os);
					if (child.type() == Type::alternative) {
						os << ")";
					}
				}
				return;
			case Type::alternative:
				for (auto &child : node.children()) {
					if (first) {
						first = false;
					} else {
						os << " | ";
					}

					debug_node(child, os);
				}
				return;
			case Type::closure:
				os << "[" << node.children()[0] << "]";
				return;
			case Type::optional:
				os << "(" << node.children()[0] << ")?";
				return;
			case Type::negation: {
				auto &child = node.children()[0];
				if (child.type() == Type::alternative || child.type() == Type::optional) {
					os << "!(";
					debug_node(child, os);
					os << ")";
				} else {
					os << "!";
					debug_node(child, os);
				}
				return;
			}
		}
	}

	void CfgContext::debug_node(std::string const &name, std::ostream &os) const {
		if (auto node = get(name)) {
			debug_node(*node, os);
		} else {
			os << "<anon node>";
		}
	}

	std::string CfgContext::node_str(CfgNode const &node) const {
		auto ss = std::stringstream();

		debug_node(node, ss);

		return ss.str();
	}

	std::string CfgContext::node_str(std::string const &name) const {
		auto ss = std::stringstream();

		debug_node(name, ss);

		return ss.str();
	}
}
