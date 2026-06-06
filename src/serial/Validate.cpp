#include "Validate.hpp"

#include "Tokenizer.hpp"
#include "codegen/AstNode.hpp"
#include "codegen/AstNodeIterator.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include <cstddef>

namespace serial {
	using T = TokenType;
	util::Result<Enumerator, Error> Enumerator::create(const Node &node) {
		log_assert(
			node.cfg_rule() == "enumerator",
			"Must pass enumerator AstNode to Enumerator::create"
		);
		
		auto e = Enumerator();
		Node const *child;
		if (auto err = node.child_with_tok(int(T::Identifier)).move_or(child)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Expecting Identifier in enumerator",
				err.value()
			);
		}

		e._name = child->consumed_all();

		if (auto err = node.child_with_tok(int(T::IntConst)).move_or(child)) {
			if (err->type() != cg::ErrorType::MISSING_AST_NODE) {
				return Error(
					ErrorType::INVALID_STATE,
					"Problem when looking up whether enumerator has value",
					err.value()
				);
			}
		} else {
			try {
				e._value = std::stoi(child->consumed_all());
			} catch (std::exception e) {
				return Error(
					ErrorType::VALIDATE_ERROR,
					util::f("Couldn't parse enumerator int: ", e.what())
				);
			}
		}

		log_trace() << "Created enumerator " << e._name << std::endl;

		return e;
	}

	TemplObj Enumerator::templ_obj() const {
		return {
			{"name", _name},
			{"value", cg::TemplInt(_value.value_or(0))},
			{"has_value", _value.has_value()}
		};
	}

	std::string const &Enumerator::name() const {
		return _name;
	}

	util::Result<Enum, Error> Enum::create(Node const &node) {
		log_assert(
			node.cfg_rule() == "enum-decl",
			"Must pass enum-decl AstNode to Enum::create"
		);

		auto e = Enum();
		Node const *child;
		if (auto err =  node.child_with_tok(int(T::Identifier)).move_or(child)) {
			return Error(ErrorType::INVALID_STATE, "Expecting Identifier in enum-decl", err.value());
		}
		e._name = child->consumed_all();

		for (auto c : node.children_with_cfg("enumerator")) {
			Enumerator en;
			if (auto err = Enumerator::create(*c).move_or(en)) {
				return Error(
					ErrorType::VALIDATE_ERROR,
					util::f("Couldn't validate enumerator of ", e._name),
					err.value()
				);
			}
			e._enumerators.push_back(en);
		}

		log_trace() << "Created enum " << e._name << std::endl;
		return e;
	}

	TemplObj Enum::templ_obj() const {
		auto enumerators = cg::TemplList();

		for (auto &e : _enumerators) {
			enumerators.push_back(e.templ_obj());
		}

		return {
			{"name", _name},
			{"enumerators", enumerators}
		};
	}

	std::string const &Enum::name() const {
		return _name;
	}

	util::Result<BFField, Error> BFField::create(Node const &node, uint8_t index) {
		log_assert(
			node.cfg_rule() == "bitfield-field",
			"Must pass bitfield-field to BFField::create"
		);
		log_assert(index > 0, "BFField index must be greater than 0");

		auto b = BFField();
		b._index = index;
		
		Node const *child;
		if (auto err = node.child_with_tok(int(T::Identifier)).move_or(child)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Expecting Identifier in bitfield-field",
				err.value()
			);
		}
		b._name = child->consumed_all();

		log_trace() << "Created bitfield field " << b._name << std::endl;

		return b;
	}

	TemplObj BFField::templ_obj() const {
		return {
			{"name", _name},
			{"index", _index}
		};
	}

	std::string const &BFField::name() const {
		return _name;
	}

	util::Result<Bitfield, Error> Bitfield::create(Node const &node) {
		log_assert(
			node.cfg_rule() == "bitfield-decl",
			"Must pass bitfield-decl to Bitfield::create"
		);

		auto b = Bitfield();

		Node const *child;
		if (auto err = node.child_with_tok(int(T::Identifier)).move_or(child)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Expecting Identifier in bitfield",
				err.value()
			);
		}
		b._name = child->consumed_all();

		auto i = 1;
		for (auto f : node.children_with_cfg("bitfield-field")) {
			BFField field;
			if (auto err = BFField::create(*f, i).move_or(field)) {
				return Error(ErrorType::VALIDATE_ERROR, util::f("Could not validate field of bitfield ", b._name), err.value());
			}
			b._fields.push_back(field);
		}

		log_trace() << "Created bitfield " << b._name << std::endl;
		return b;
	}

	TemplObj Bitfield::templ_obj() const {
		auto fields = cg::TemplList();
		for (auto &f : _fields) {
			fields.push_back(f.templ_obj());
		}

		return {
			{"name", _name},
			{"fields", fields}
		};
	}

	std::string const &Bitfield::name() const {
		return _name;
	}

	util::Result<VersionValue, Error> VersionValue::create(Node const &node) {
		log_assert(node.cfg_rule() == "version-frag", "Must pass version-frag to VersionValue::create");

		auto v = VersionValue();
		auto nums = node.children_with_tok(int(T::IntConst));
		if (nums.size() != 3) {
			return Error(ErrorType::INVALID_STATE, "version-frag must contain 3 child IntConsts");
		}
		try {
			v.vanity = std::stoi(nums[0]->consumed_all());
			v.major = std::stoi(nums[1]->consumed_all());
			v.minor = std::stoi(nums[2]->consumed_all());
		} catch (std::exception const &e) {
			return Error(ErrorType::INVALID_STATE, util::f("Couldn't parse version numbers: ", e.what()));
		}

		log_trace() << "Created version value " << v.vanity << "." << v.major << "." << v.minor << std::endl;

		return v;
	}

	std::string VersionValue::namespace_str() const {
		return util::f("v", vanity, "_", major, "_", minor);
	}

	bool VersionValue::operator<(VersionValue const &other) const {
		if (vanity != other.vanity) {
			return vanity < other.vanity;
		}
		if (major != other.major) {
			return major < other.major;
		}
		return minor < other.minor;
	}

	util::Result<Version, Error> Version::create(Node const &node) {
		Version v;
		log_assert(node.cfg_rule() == "version-decl", "Must pass version-decl AstNode to Version::create");

		Node *value_node;
		if (auto err = node.child_with_cfg("version-frag").move_or(value_node)) {
			return Error(ErrorType::INVALID_STATE, "version-decl does not contain a version-frag child node", err.value());
		}
		auto value = VersionValue();
		if (auto err = VersionValue::create(*value_node).move_or(value)) {
			return Error(ErrorType::VALIDATE_ERROR, "Could not validate version fragment", err.value());
		}

		Node *blck;
		if (auto err = node.child_with_cfg("version-blck").move_or(blck)) {
			return Error(ErrorType::INVALID_STATE, "version-decl does not contain a version-blck child node", err.value());
		}

		for (auto &child : *blck) {
			if (child.cfg_rule() == "enum-decl") {
				auto e = Enum();
				if (auto err = Enum::create(child).move_or(e)) {
					return Error(ErrorType::VALIDATE_ERROR, util::f("Could not validate enum in version ", value.namespace_str()), err.value());
				}
				if (auto err = v._check_identifier(e.name()).move_or()) {
					return err.value();
				}
				v._enums[e.name()] = e;
			} else if (child.cfg_rule() == "bitfield-decl") {
				auto b = Bitfield();
				if (auto err = Bitfield::create(child).move_or(b)) {
					return Error(ErrorType::VALIDATE_ERROR, util::f("Could not validate bitfield in version ", value.namespace_str()), err.value());
				}
				if (auto err = v._check_identifier(b.name()).move_or()) {
					return err.value();
				}
				v._bitfields[b.name()] = b;
			} else if (child.cfg_rule() == "struct-decl") {
				log_warning() << "TODO" << std::endl;
			}
		}

		return {v};
	}

	TemplObj Version::templ_obj() const {
		auto enums = cg::TemplList();
		for (auto &[name, e] : _enums) {
			enums.push_back(e.templ_obj());
		}

		auto bitfields = cg::TemplList();
		for (auto &[name, b] : _bitfields) {
			bitfields.push_back(b.templ_obj());
		}

		return {
			{"namespace_str", _value.namespace_str()},
			{"enums", enums},
			{"bitfields", bitfields}
		};
	}

	VersionValue const &Version::value() const {
		return _value;
	}

	util::Result<void, Error> Version::_check_identifier(
		std::string const &name,
		util::FileLocation floc
	) const {
		if (_enums.contains(name)) {
			return Error(
				ErrorType::VALIDATE_ERROR,
				util::f(name, " is already defined for the current version"),
				floc
			);
		} else if (_bitfields.contains(name)) {
			return Error(
				ErrorType::VALIDATE_ERROR,
				util::f(name, " is already defined for the current version"),
				floc
			);
		}
		//TODO: check classes
		return {};
	}

	util::Result<void, Error> Document::add_file(Node const &node) {
		log_assert(node.cfg_rule() == "root", "Must pass root AstNode to Document::add_file");

		Node *doc_node;
		if (auto err = node.child_with_cfg("document").move_or(doc_node)) {
			return Error(ErrorType::INVALID_STATE, "Expecting root to contain a document child", err.value());
		}

		for (auto child : *doc_node) {
			if (child.cfg_rule() == "include-stmt") {
				Node *str_node;
				if (auto err = child.child_with_tok(int(T::StrConst)).move_or(str_node)) {
					return Error(ErrorType::INVALID_STATE, "Expecting include-stmt to have a StrConst child node", err.value());
				}
				auto str = std::string();
				if (auto err = util::unescape_str(child.consumed_all()).move_or(str)) {
					return Error(ErrorType::PARSE_ERROR, util::f("Could not evaluate string: ", child.consumed_all()), err.value());
				}
				_includes.push_back(str);
			} else if (child.cfg_rule() == "version-decl") {
				Version v;
				if (auto err = Version::create(child).move_or(v)) {
					return Error(ErrorType::PARSE_ERROR, "Could not parse version", err.value());
				}
				_versions[v.value()] = v;
			}
		}
		return {};
	}

	std::vector<std::string> const &Document::includes() const {
		return _includes;
	}

	TemplObj Document::templ_obj() const {
		auto versions = cg::TemplList();
		for (auto &[name, v] : _versions) {
			versions.push_back(v.templ_obj());
		}
		return {
			{"versions", versions}
		};
	}
}
