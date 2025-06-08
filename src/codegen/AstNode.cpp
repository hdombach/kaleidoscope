#include "AstNode.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"

namespace cg {
	AstNode::AstNode():
		_type(Type::None),
		_id(0),
		_size(0)
	{}

	AstNode AstNode::create_rule(
		uint32_t id,
		std::string const &cfg_name,
		util::FileLocation const &file_location
	) {
		AstNode r;
		r._type = Type::Rule;
		r._id = id;
		r._cfg_rule = cfg_name;
		r._location = file_location;
		r._size = 0;
		return r;
	}

	AstNode AstNode::create_str(
		uint32_t id,
		std::string const &str,
		util::FileLocation const &file_location
	) {
		AstNode r;
		r._type = Type::String;
		r._id = id;
		r._consumed = str;
		r._location = file_location;
		r._size = 0;
		return r;
	}

	bool AstNode::has_value() const {
		return _type != Type::None;
	}

	void AstNode::add_child(AstNode const &node) {
		log_assert(_type == Type::Rule, "Can't add a child to an AstNode which is not a rule.");
		_children.push_back(node);
		_size = 0; /* Cache is invalidated */
	}

	util::Result<AstNode, KError> AstNode::child_with_cfg(std::string const &name) const {
		for (auto &child : _children) {
			if (child._cfg_rule == name) {
				return child;
			}
		}
		return KError::codegen(util::f("CfgNode does not contain child with name ", name));
	}

	std::vector<AstNode> AstNode::children_with_cfg(std::string const &name) const {
		auto result = std::vector<AstNode>();
		for (auto &child : _children) {
			if (child._cfg_rule == name) {
				result.push_back(child);
			}
		}
		return result;
	}

	std::string AstNode::consumed_all() const {
		auto s = std::string();
		if (_type == Type::Rule) {
			for (auto &c : _children) {
				s += c.consumed_all();
			}
		} else {
			s = consumed();
		}
		return s;
	}

	size_t AstNode::size() const {
		if (_size == 0) {
			_size = _calc_size();
		}
		return _size;
	}

	util::FileLocation AstNode::location() const {
		return _location;
	}

	void AstNode::compress(std::set<std::string> const &cfg_names) {
		auto new_children = std::vector<AstNode>();
		for (auto &child : _children) {
			child.compress(cfg_names);
			if (child.type() == Type::Rule) {
				auto f = std::find(
					cfg_names.begin(),
					cfg_names.end(),
					child.cfg_rule()
				);
				if (f == cfg_names.end()) {
					new_children.insert(
						new_children.end(),
						child.children().begin(),
						child.children().end()
					);
				} else {
					new_children.push_back(child);
				}
			} else {
				new_children.push_back(child);
			}
		}
		_children = new_children;
	}

	AstNode AstNode::compressed(std::set<std::string> const &cfg_names) const {
		auto result = *this;
		result.compress(cfg_names);
		return result;
	}

	void AstNode::trim() {
		auto new_children = std::vector<AstNode>();

		for (auto &child : _children) {
			child.trim();
			if (child.consumed().size() > 0 || child.children().size() > 0) {
				new_children.push_back(child);
			}
		}

		_children = new_children;
	}

	AstNode AstNode::trimmed() const {
		auto r = *this;
		r.trim();
		return r;
	}

	std::ostream &AstNode::print_debug(std::ostream &os) const {
		if (_children.empty()) {
			os << '"' << consumed() << '"';
		} else {
			os << "{";
			os << '"' << consumed() << '"' << ": ";
			os << util::plist(_children);
			os << "}";
		}
		return os;
	}

	std::ostream &AstNode::print_pre_order(std::ostream &os) const {
		os << _cfg_rule << " ";
		for (auto &child : children()) {
			child.print_pre_order(os);
		}
		return os;
	}

	std::ostream &AstNode::print_dot(
		std::ostream &os,
		std::string const &name
	) const {
		os << "digraph graphname {" << std::endl;
		if (!_cfg_rule.empty()) {
			os << "label=\"" << name << "\"\n";
		}
		_print_dot_attributes(os);
		_print_dot_paths(os);
		os << "}" << std::endl;
		return os;
	}

	size_t AstNode::_calc_size() const {
		auto s = _consumed.size();
		for (auto &c : _children) {
			s += c.size();
		}
		return s;
	}

	void AstNode::_print_dot_attributes(std::ostream &os) const {
		os << "ast_" << id() << " [label=\"";
		if (_type == Type::Rule) {
			log_assert(!_cfg_rule.empty(), "A rule AstNode must have valid cfg name");
			os << "<" << _cfg_rule << ">";
		} else if (_type == Type::String) {
			log_assert(_children.empty(), "A string AstNode must have now children");
			os << "\\\"" << util::escape_str(_consumed) << "\\\"" << std::endl;
		} else {
			os << "none" << std::endl;
		}
		os << "\"];" << std::endl;

		for (auto const &child : children()) {
			child._print_dot_attributes(os);
		}
	}

	void AstNode::_print_dot_paths(std::ostream &os) const {
		for (auto const &child : children()) {
			os << "ast_" << id() << " -> ast_" << child.id() << ";" << std::endl;
			child._print_dot_paths(os);
		}
	}
}
