#include "AstNode.hpp"
#include "CfgNode.hpp"

namespace cg {
	AstNode::AstNode():
		_ctx(nullptr),
		_size(0)
	{}

	AstNode::AstNode(
		uint32_t id,
		CfgContext const &ctx,
		uint32_t cfg_id,
		util::FileLocation const &file_location
	) {
		_ctx = &ctx;
		_size = 0;
		_id = id;
		_cfg_id = cfg_id;
		_location = file_location;
	}

	bool AstNode::has_value() const {
		return _ctx;
	}

	void AstNode::add_child(AstNode const &node) {
		_children.push_back(node);
		_size = 0; /* Cache is invalidated */
	}

	util::Result<AstNode, KError> AstNode::child_with_cfg(std::string const &name) const {
		for (auto &child : _children) {
			if (child.cfg_name() == name) {
				return child;
			}
		}
		return KError::codegen(util::f("CfgNode does not contain child with name ", name));
	}

	std::vector<AstNode> AstNode::children_with_cfg(std::string const &name) const {
		auto result = std::vector<AstNode>();
		for (auto &child : _children) {
			if (child.cfg_name() == name) {
				result.push_back(child);
			}
		}
		return result;
	}

	void AstNode::consume(char c) {
		_consumed.push_back(c);
		_size = 0; /* Invalidate cache */
	}

	size_t AstNode::size() const {
		if (_size == 0) {
			_size = _calc_size();
		}
		return _size;
	}

	bool AstNode::is_ref() const {
		return _ctx->get(_cfg_id).type() == CfgNode::Type::reference;
	}

	util::FileLocation AstNode::location() const {
		return _location;
	}

	void AstNode::compress(std::vector<uint32_t> const &cfg_ids) {
		auto new_children = std::vector<AstNode>();
		for (auto &child : _children) {
			child.compress(cfg_ids);
			if (std::find(cfg_ids.begin(), cfg_ids.end(), child._cfg_id) == cfg_ids.end()) {
				//Combine the current child into slef
				_consumed += child._consumed;
				new_children.insert(new_children.end(), child.children().begin(), child.children().end());
			} else {
				//Keep the current child if it is not empty
				if (!child.consumed().empty() || child.children().size() > 0) {
					new_children.push_back(child);
				}
			}
		}
		_children = new_children;
	}

	util::Result<void, KError> AstNode::compress() {
		auto const &cfg_names = _ctx->prim_names();
		auto ids = std::vector<uint32_t>();
		for (auto &name : cfg_names) {
			if (!_ctx->contains(name)) {
				return KError::codegen("Internal: Unknown cfg name in compress: " + name);
			}
			ids.push_back(_ctx->get(name).id());
		}
		compress(ids);
		return {};
	}

	AstNode AstNode::compressed(std::vector<uint32_t> const &cfg_ids) {
		auto result = *this;
		result.compress(cfg_ids);
		return result;
	}

	util::Result<AstNode, KError> AstNode::compressed() {
		auto result = *this;
		TRY(result.compress());
		return result;
	}

	void AstNode::debug_pre_order(std::ostream &os) const {
		os << cfg_node().name() << " ";
		for (auto &child : children()) {
			child.debug_pre_order(os);
		}
	}

	void AstNode::debug_dot(std::ostream &os) const {
		os << "digraph graphname {" << std::endl;
		_debug_dot_attributes(os);
		_debug_dot_paths(os);
		os << "}" << std::endl;
	}

	std::string AstNode::pre_order_str() const {
		std::stringstream ss;
		debug_pre_order(ss);
		return ss.str();
	}
	
	size_t AstNode::_calc_size() const {
		auto s = _consumed.size();
		for (auto &c : _children) {
			s += c.size();
		}
		return s;
	}

	void AstNode::_debug_dot_attributes(std::ostream &os) const {
		auto desc = cfg_node().name() + ": " + _ctx->node_str(cfg_node().name());

		os << "ast_" << id() << " [label=\"";
		if (cfg_node().name().empty()) {
			os << "<anon>";
		} else {
			os << "<" << cfg_node().name() << ">";
		}
		if (!consumed().empty()) {
			os << ": ";
			os << "\\\"" << util::escape_str(consumed()) << "\\\"";
		}
		os << "\"];" << std::endl;
		for (auto const &child : children()) {
			child._debug_dot_attributes(os);
		}
	}

	void AstNode::_debug_dot_paths(std::ostream &os) const {
		for (auto const &child : children()) {
			os << "ast_" << id() << " -> ast_" << child.id() << ";" << std::endl;
			child._debug_dot_paths(os);
		}
	}
}
