#include "AST.hpp"
#include "CFG.hpp"
#include "util/Util.hpp"

namespace cg {
	ASTNode::ASTNode():
		_cfg(nullptr),
		_size(0)
	{ }

	ASTNode::ASTNode(Cfg const &cfg, uint32_t id) {
		_cfg = &cfg;
		_size = 0;
		_id = id;
	}

	bool ASTNode::has_value() const {
		return _cfg;
	}

	void ASTNode::add_child(ASTNode const &node) {
		_children.push_back(node);
		/* Cache is invalidated */
		_size = 0;
	}

	void ASTNode::consume(char c) {
		_consumed.push_back(c);
		_size = 0;
	}

	size_t ASTNode::size() const {
		if (_size == 0) {
			_size = _calc_size();
		}
		return _size;
	}

	bool ASTNode::is_ref() const {
		return _cfg->type() == Cfg::Type::reference;
	}

	void ASTNode::compress(std::vector<Cfg const *> cfgs) {
		auto new_children = std::vector<ASTNode>();
		for (auto &child : _children) {
			child.compress(cfgs);
			if (std::find(cfgs.begin(), cfgs.end(), &child.cfg()) == cfgs.end()) {
				//Combine the current child into self
				_consumed += child._consumed;
				new_children.insert(new_children.end(), child.children().begin(), child.children().end());
			} else {
				//Keep the current child if it is non empty
				if (!child.consumed().empty() || child.children().size() > 0) {
					new_children.push_back(child);
				}
			}
		}
		_children = new_children;
	}

	ASTNode ASTNode::compressed(std::vector<Cfg const *> cfgs) const {
		auto result = *this;
		result.compress(cfgs);
		return result;
	}

	size_t ASTNode::_calc_size() const {
		auto s = _consumed.size();
		for (auto &c : _children) {
			s += c.size();
		}
		return s;
	}

	util::Result<size_t, ASTError> match_cfg_literal(const char *str, Cfg const &cfg) {
		auto r = 0;
		for (auto c : cfg.content()) {
			if (str[r] != c) return ASTError{};
			r++;
		}
		return r;
	}

	util::Result<size_t, ASTError> match_cfg_ref(const char *str, Cfg const &cfg) {
		return match_cfg(str, cfg.ref());
	}

	util::Result<size_t, ASTError> match_cfg_seq(const char *str, Cfg const &cfg) {
		size_t r = 0;
		for (auto &child : cfg.children()) {
			if (auto i = match_cfg(str+r, child)) {
				r += i.value();
			} else {
				return ASTError{};
			}
		}
		return r;
	}

	util::Result<size_t, ASTError> match_cfg_alt(const char *str, Cfg const &cfg) {
		for (auto &child : cfg.children()) {
			if (auto i = match_cfg(str, child)) {
				return i;
			}
		}
		return ASTError{};
	}

	util::Result<size_t, ASTError> match_cfg_closure(const char *str, Cfg const &cfg) {
		size_t r = 0;
		while (true) {
			if (auto i = match_cfg(str + r, cfg.children()[0])) {
				r += i.value();
			} else {
				return r;
			}
		}
	}

	util::Result<size_t, ASTError> match_cfg_opt(const char *str, Cfg const &cfg) {
		return match_cfg(str, cfg.children()[0]).value(0);
	}

	util::Result<size_t, ASTError> match_cfg(const char *str, Cfg const &cfg) {
		switch (cfg.type()) {
			case Cfg::Type::none:
				return 0;
			case Cfg::Type::literal:
				return match_cfg_literal(str, cfg);
			case Cfg::Type::reference:
				return match_cfg_ref(str, cfg);
			case Cfg::Type::sequence:
				return match_cfg_seq(str, cfg);
			case Cfg::Type::alternative:
				return match_cfg_alt(str, cfg);
			case Cfg::Type::closure:
				return match_cfg_closure(str, cfg);
			case Cfg::Type::optional:
				return match_cfg_opt(str, cfg);
		}
	}

	//TODO: make this more official
	static uint32_t g_uid = 1;

	/************************************************
	 *                  Parsing
	 ************************************************/

	util::Result<ASTNode, ASTError> parse_cfg_literal(
		const char *str,
		Cfg const &cfg
	) {
		size_t r = 0;
		auto node = ASTNode(cfg, g_uid++);
		for (auto c : cfg.content()) {
			if (str[r] != c) return ASTError{};
			node.consume(c);
			r++;
		}
		return {node};
	}

	util::Result<ASTNode, ASTError> parse_cfg_ref(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg, g_uid++);

		if (auto child = parse_cfg(str, cfg.ref())) {
			node.add_child(child.value());
		} else {
			return ASTError();
		}
		return node;
	}

	util::Result<ASTNode, ASTError> parse_cfg_seq(
		const char *str,
		Cfg const &cfg
	) {
		size_t r = 0;
		auto node = ASTNode(cfg, g_uid++);
		for (auto &child_cfg : cfg.children()) {
			if (auto child = parse_cfg(str+r, child_cfg)) {
				r += child.value().size();
				node.add_child(child.value());
			} else {
				return ASTError{};
			}
		}
		return node;
	}

	util::Result<ASTNode, ASTError> parse_cfg_alt(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg, g_uid++);
		for (auto &child_cfg : cfg.children()) {
			if (auto child = parse_cfg(str, child_cfg)) {
				node.add_child(child.value());
				return node;
			}
		}
		return ASTError{};
	}

	util::Result<ASTNode, ASTError> parse_cfg_closure(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg, g_uid++);
		size_t r = 0;
		while (true) {
			if (auto child = parse_cfg(str + r, cfg.children()[0])) {
				r += child.value().size();
				node.add_child(child.value());
			} else {
				return node;
			}
		}
	}

	util::Result<ASTNode, ASTError> parse_cfg_opt(
		const char *str,
		Cfg const &cfg
	) {
		auto node = ASTNode(cfg, g_uid++);
		if (auto child = parse_cfg(str, cfg.children()[0])) {
			node.add_child(child.value());
		}
		return node;
	}

	util::Result<ASTNode, ASTError> parse_cfg(const char *str, Cfg const &cfg) {
		switch (cfg.type()) {
			case Cfg::Type::none:
				return ASTError{};
			case Cfg::Type::literal:
				return parse_cfg_literal(str, cfg);
			case Cfg::Type::reference:
				return parse_cfg_ref(str, cfg);
			case Cfg::Type::sequence:
				return parse_cfg_seq(str, cfg);
			case Cfg::Type::alternative:
				return parse_cfg_alt(str, cfg);
			case Cfg::Type::closure:
				return parse_cfg_closure(str, cfg);
			case Cfg::Type::optional:
				return parse_cfg_opt(str, cfg);
		}
	}

	std::ostream &ASTNode::debug_pre_order(std::ostream &os) const {
		os << cfg().name() << " ";
		for (auto &child : children()) {
			child.debug_pre_order(os);
		}
		return os;
	}

	void _debug_dot_attributes(ASTNode const &node, std::ostream &os) {

		auto desc = node.cfg().name() + ": " + node.cfg().str();


		os << "ast_" << node.id() << " [label=\"";
		if (node.cfg().name().empty()) {
			os << "<anon>";
		} else {
			os << "<" << node.cfg().name() << ">";
		}
		if (!node.consumed().empty()) {
			os << ": ";
			os << "\\\"" << util::escape_str(node.consumed()) << "\\\"";
		}
		os << "\"];" << std::endl;
		for (auto const &child : node.children()) {
			_debug_dot_attributes(child, os);
		}
	}

	void _debug_dot_paths(ASTNode const &node, std::ostream &os) {
		for (auto const &child : node.children()) {
			os << "ast_" << node.id() << " -> ast_" << child.id() << ";" << std::endl;
			_debug_dot_paths(child, os);
		}
	}

	void ASTNode::debug_dot(std::ostream &os) const {
		os << "digraph graphname {" << std::endl;
		_debug_dot_attributes(*this, os);
		_debug_dot_paths(*this, os);
		os << "}" << std::endl;
	}

	std::string ASTNode::pre_order_str() const {
		std::stringstream ss;
		debug_pre_order(ss);
		return ss.str();
	}
}
