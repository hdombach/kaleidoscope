#include "AstNode.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"
#include "AstNodeIterator.hpp"

namespace cg {
	Token _default_token;
	AstNode::AstNode():
		_type(Type::None),
		_id(0),
		_token(&_default_token),
		_sibling_prev(nullptr),
		_sibling_next(nullptr),
		_child_head(nullptr),
		_child_tail(nullptr)
	{}

	AstNode::AstNode(AstNode const &other):
		_type(other._type),
		_id(other._id),
		_cfg_rule(other._cfg_rule),
		_token(other._token),
		_sibling_prev(other._sibling_prev),
		_sibling_next(other._sibling_next),
		_child_head(other._child_head),
		_child_tail(other._child_tail)
	{}

	AstNode::AstNode(AstNode &&other):
		_type(other._type),
		_id(other._id),
		_cfg_rule(std::move(other._cfg_rule)),
		_token(other._token),
		_sibling_prev(other._sibling_prev),
		_sibling_next(other._sibling_next),
		_child_head(other._child_head),
		_child_tail(other._child_tail)
	{
		other._type = Type::None;
		other._id = 0;
		other._token = &_default_token;
		other._sibling_prev = nullptr;
		other._sibling_next = nullptr;
		other._child_head = nullptr;
		other._child_tail = nullptr;
	}

	AstNode &AstNode::operator=(AstNode const &other) {
		_type = other._type;
		_id = other._id;
		_cfg_rule = other._cfg_rule;
		_token = other._token;
		_sibling_prev = other._sibling_prev;
		_sibling_next = other._sibling_next;
		_child_head = other._child_head;
		_child_tail = other._child_tail;
		return *this;
	}

	AstNode &AstNode::operator=(AstNode &&other) {
		_type = other._type;
		_id = other._id;
		_cfg_rule = std::move(other._cfg_rule);
		_token = other._token;
		_sibling_prev = other._sibling_prev;
		_sibling_next = other._sibling_next;
		_child_head = other._child_head;
		_child_tail = other._child_tail;

		other._type = Type::None;
		other._id = 0;
		other._token = &_default_token;
		other._sibling_prev = nullptr;
		other._sibling_next = nullptr;
		other._child_head = nullptr;
		other._child_tail = nullptr;
		return *this;
	}

	AstNode AstNode::create_rule(
		uint32_t id,
		std::string const &cfg_name
	) {
		AstNode r;
		r._type = Type::Rule;
		r._id = id;
		r._cfg_rule = cfg_name;
		return r;
	}

	AstNode AstNode::create_tok(
		uint32_t id,
		Token const &token
	) {
		AstNode r;
		r._type = Type::Leaf;
		r._id = id;
		r._token = &token;
		return r;
	}

	bool AstNode::has_value() const {
		return _type != Type::None;
	}

	AstNodeIterator AstNode::begin() {
		return AstNodeIterator(_child_head);
	}

	AstNodeIterator AstNode::begin() const {
		return AstNodeIterator(_child_head);
	}

	AstNodeIterator AstNode::end() {
		return AstNodeIterator(nullptr);
	}

	AstNodeIterator AstNode::end() const {
		return AstNodeIterator(nullptr);
	}

	void AstNode::add_child(AstNode &node) {
		log_assert(_type == Type::Rule, "Can't add a child to an AstNode which is not a rule.");

		if (_child_head) {
			_child_tail->_sibling_next = &node;
			node._sibling_prev = _child_tail;
			_child_tail = &node;
		} else {
			_child_head = &node;
			_child_tail = &node;
		}
	}

	util::Result<AstNode*, Error> AstNode::child_with_cfg(std::string const &name) const {
		for (auto &child : *this) {
			if (child._cfg_rule == name) {
				return &child;
			}
		}
		return Error(ErrorType::MISSING_AST_NODE, util::f("AstNode ", str(), " does not contain child with name \"", name, "\""));
	}

	std::vector<AstNode*> AstNode::children_with_cfg(std::string const &name) const {
		auto result = std::vector<AstNode*>();
		for (auto &child : *this) {
			if (child._cfg_rule == name) {
				result.push_back(&child);
			}
		}
		return result;
	}

	util::Result<AstNode*, Error> AstNode::child_with_tok(Token::Type type) const {
		for (auto &child : *this) {
			if (child.tok().type() == type) {
				return &child;
			}
		}
		return Error(ErrorType::MISSING_AST_NODE, util::f("AstNode ", str(), " does not contain child with type ", type));
	}

	Token const &AstNode::tok() const {
		return *_token;
	}

	std::string AstNode::consumed_all() const {
		auto s = std::string();
		if (_type == Type::Rule) {
			for (auto &c : *this) {
				s += c.consumed_all();
			}
		} else {
			s = tok().content();
		}
		return s;
	}

	size_t AstNode::leaf_count() const {
		if (_type == Type::Leaf) return 1;
		size_t result = 0;
		for (auto &child : *this) {
			result += child.leaf_count();
		}
		return result;
	}

	size_t AstNode::child_count() const {
		size_t i = 0;
		auto n = _child_head;
		while (n) {
			n = n->_sibling_next;
			i++;
		}
		return i;
	}

	util::FileLocation AstNode::location() const {
		if (_type == Type::Leaf) {
			return _token->loc();
		} else {
			log_assert(_child_head, "Rule AstNode must have children");
			return _child_head->location();
		}
	}

	void AstNode::compress(std::set<std::string> const &cfg_names) {
		auto n = begin();
		while (n != end()) {
			n->compress(cfg_names);
			if (n->type() == Type::Rule) {
				auto f = std::find(
					cfg_names.begin(),
					cfg_names.end(),
					n->cfg_rule()
				);
				if (f == cfg_names.end()) {
					//Replace current n with all its children
					//Dont' bother deleting astnode. it will just be unused.
					if (n->_sibling_prev == nullptr) {
						_child_head = n->_child_head;
					} else {
						n->_sibling_prev->_sibling_next = n->_child_head;
						n->_child_head->_sibling_prev = n->_sibling_prev;
					}
					if (n->_sibling_next == nullptr) {
						_child_tail = n->_child_tail;
					} else {
						n->_sibling_next->_sibling_prev = n->_child_tail;
						n->_child_tail->_sibling_next = n->_sibling_next;
					}
				}
			}
			n++;
		}
	}

	void AstNode::trim() {
		for (auto &child : *this) {
			child.trim();
			if (!child.tok().exists() && child.begin() == child.end()) {
				if (child._sibling_next) {
					child._sibling_next = child._sibling_next->_sibling_next;
					child._sibling_next->_sibling_prev = &child;
				}
			}
		}
	}

	AstNode AstNode::trimmed() const {
		auto r = *this;
		r.trim();
		return r;
	}

	std::ostream &AstNode::print_debug(std::ostream &os) const {
		std::string str;
		if (_type == Type::Leaf) {
			str = util::escape_str(tok().content());
		} else if (_type == Type::Rule) {
			str = _cfg_rule;
		}
		if (begin() == end()) {
			os << '"' << str << '"';
		} else {
			os << "{";
			os << '"' << str << '"' << ": ";
			os << util::plist(*this);
			os << "}";
		}
		return os;
	}

	std::ostream &AstNode::print_pre_order(std::ostream &os) const {
		if (_type == Type::Leaf) {
			os << tok().type();
		} else {
			os << _cfg_rule;
		}
		os << " ";
		for (auto &child : *this) {
			child.print_pre_order(os);
		}
		return os;
	}

	std::ostream &AstNode::print_src(std::ostream &os) const {
		if (_type == Type::Leaf) {
			os << tok().content();
		} else {
			for (auto &child : *this) {
				child.print_src(os);
			}
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

	std::string AstNode::str() const {
		auto ss = std::stringstream();
		print_debug(ss);
		return ss.str();
	}

	std::string AstNode::str_src() const {
		auto ss = std::stringstream();
		print_src(ss);
		return ss.str();
	}

	void AstNode::_print_dot_attributes(std::ostream &os) const {
		os << "ast_" << id() << " [label=\"";
		if (_type == Type::Rule) {
			log_assert(!_cfg_rule.empty(), "A rule AstNode must have valid cfg name");
			os << "<" << _cfg_rule << ">";
		} else if (_type == Type::Leaf) {
			log_assert(begin() == end(), "A string AstNode must have no children");
			os << "\\\"" << util::escape_str(_token->content()) << "\\\"" << std::endl;
		} else {
			os << "none" << std::endl;
		}
		os << "\"];" << std::endl;

		for (auto const &child : *this) {
			child._print_dot_attributes(os);
		}
	}

	void AstNode::_print_dot_paths(std::ostream &os) const {
		for (auto const &child : *this) {
			os << "ast_" << id() << " -> ast_" << child.id() << ";" << std::endl;
			child._print_dot_paths(os);
		}
	}
}
