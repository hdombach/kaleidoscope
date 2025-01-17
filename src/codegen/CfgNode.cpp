#include "CfgNode.hpp"

#include "util/log.hpp"

namespace cg {
	CfgNode::CfgNode(): _type(Type::none), _id(0), _ref_id(0) {}

	CfgNode::CfgNode(CfgNode &&other) {
		_id = other._id;
		other._id = 0;

		_name = std::move(other._name);

		_type = other._type;
		other._type = Type::none;

		_children = std::move(other._children);
		_content = std::move(other._content);

		_ref_id = other._ref_id;
		other._ref_id = 0;

		_ref_name = std::move(other._ref_name);
	}

	CfgNode &CfgNode::operator=(CfgNode &&other) {
		destroy();

		_id = other._id;
		other._id = 0;

		_name = std::move(other._name);

		_type = other._type;
		other._type = Type::none;

		_children = std::move(other._children);
		_content = std::move(other._content);

		_ref_id = other._ref_id;
		other._ref_id = 0;

		_ref_name = std::move(other._ref_name);

		return *this;
	}

	CfgNode CfgNode::literal(std::string const &str) {
		auto result = CfgNode();

		result._type = Type::literal;
		result._content = str;

		return result;
	}

	CfgNode CfgNode::ref(uint32_t ref_id) {
		auto result = CfgNode();

		result._type = Type::reference;
		result._ref_id = ref_id;

		return result;
	}

	CfgNode CfgNode::ref(std::string const &ref_name) {
		auto result = CfgNode();

		result._type = Type::reference;
		result._ref_name = ref_name;

		return result;
	}

	CfgNode CfgNode::ref(CfgNode const &other) {
		auto result = CfgNode();

		result._type = Type::reference;
		// There will be at least one of these
		result._ref_id = other.id();
		result._ref_name = other.name();
		log_assert(other.name().size() > 0, "ref does not have name");

		return result;
	}

	CfgNode CfgNode::seq(CfgNode &&lhs, CfgNode &&rhs) {
		auto result = CfgNode();

		if (lhs.type() == Type::literal && rhs.type() == Type::literal) {
			/* Concat to literals */
			result._type = Type::literal;
			result._content = lhs.content() + rhs.content();

			return result;
		}

		/* Remove empties */
		if (lhs.type() == Type::none) {
			return rhs;
		}
		if (rhs.type() == Type::none) {
			return lhs;
		}

		result._type = Type::sequence;

		/* Combine neighbor sequences */
		if (lhs.type() == Type::sequence) {
			result._children = std::move(lhs._children);
		} else {
			result._children.push_back(std::move(lhs));
		}

		if (rhs.type() == Type::sequence) {
			for (auto &child : rhs._children) {
				result._children.push_back(std::move(child));
			}
		} else {
			result._children.push_back(std::move(rhs));
		}

		lhs.destroy();
		rhs.destroy();

		return result;
	}

	CfgNode CfgNode::alt(CfgNode &&lhs, CfgNode &&rhs) {
		auto result = CfgNode();

		result._type = Type::alternative;

		/* Remove empties */
		if (lhs.type() == Type::none) {
			return rhs;
		}
		if (rhs.type() == Type::none) {
			return lhs;
		}

		/* Cmbine neighbor alternative */
		if (lhs.type() == Type::alternative) {
			result._children = std::move(lhs._children);
		} else {
			result._children.push_back(std::move(lhs));
		}

		if (rhs.type() == Type::alternative) {
			for (auto &child : rhs._children) {
				result._children.push_back(std::move(child));
			}
		} else {
			result._children.push_back(std::move(rhs));
		}

		return result;
	}

	CfgNode CfgNode::cls(CfgNode &&c) {
		auto result = CfgNode();

		result._type = Type::closure;
		result._children.push_back(std::move(c));

		return result;
	}

	CfgNode CfgNode::opt(CfgNode &&c) {
		auto result = CfgNode();

		result._type = Type::optional;
		result._children.push_back(std::move(c));

		return result;
	}

	CfgNode CfgNode::neg(CfgNode &&c) {
		auto result = CfgNode();

		result._type = Type::negation;
		result._children.push_back(std::move(c));

		return result;
	}

	void CfgNode::destroy() {
		_id = 0;
		_name.clear();
		_type = Type::none;
		_children.clear();
		_content.clear();

		_ref_id = 0;
		_ref_name.clear();
	}

	bool CfgNode::has_value() const {
		return _type != Type::none;
	}

	CfgNode CfgNode::dup() const {
		auto result = CfgNode();

		result._id = _id;
		result._name = _name;
		result._type = _type;
		for (auto &child : _children) {
			result._children.push_back(child.dup());
		}
		result._content = _content;

		result._ref_id = _ref_id;
		result._ref_name = _ref_name;

		return result;
	}

	bool CfgNode::has_name() const {
		return !_name.empty();
	}

	void CfgNode::add_name(uint32_t id, std::string const &name) {
		_id = id;
		_name = name;
	}

	void CfgNode::set_ref_id(uint32_t ref_id) { _ref_id = ref_id; }
}
