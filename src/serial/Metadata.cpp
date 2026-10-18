#include "Metadata.hpp"
#include "Tokenizer.hpp"
#include "util/format.hpp"
#include "util/log.hpp"
#include "codegen/AstNode.hpp"
#include "codegen/AstNodeIterator.hpp"

namespace serial::meta {
	Type _get_type(cg::Token const &tok) {
		using T = TokenType;
		switch (T(tok.type())) {
			case T::U8:
				return Type::U8;
			case T::U16:
				return Type::U16;
			case T::U32:
				return Type::U32;
			case T::U64:
				return Type::U64;
			case T::I8:
				return Type::I8;
			case T::I16:
				return Type::I16;
			case T::I32:
				return Type::I32;
			case T::I64:
				return Type::I64;
			case T::Float:
				return Type::Float;
			case T::Double:
				return Type::Double;
			case T::Boolean:
				return Type::Boolean;
			case T::String:
				return Type::String;
			case T::Array:
				return Type::Array;
			case T::Optional:
				return Type::Optional;
			case T::UIDList:
				return Type::UIDList;
			case T::Identifier:
				return Type::UNRESOLVED;
			default:
				return Type::UNKNOWN;
		};
	}

	bool _is_prim(Type type) {
		switch (type) {
			case Type::U8:
			case Type::U16:
			case Type::U32:
			case Type::U64:
			case Type::I8:
			case Type::I16:
			case Type::I32:
			case Type::I64:
			case Type::Float:
			case Type::Double:
			case Type::Boolean:
			case Type::Enum:
			case Type::Bitfield:
			case Type::String:
				return true;
			default:
				return false;
		}
	}

	const char *_cpp_frag_str(Type type) {
		switch (type) {
			case Type::Float:
				return "float";
			case Type::Double:
				return "double";
			case Type::U8:
				return "uint8_t";
			case Type::U16:
				return "uint16_t";
			case Type::U32:
				return "uint32_t";
			case Type::U64:
				return "uint64_t";
			case Type::I8:
				return "int8_t";
			case Type::I16:
				return "int16_t";
			case Type::I32:
				return "int32_t";
			case Type::I64:
				return "int64_t";
			case Type::String:
				return "::std::string";
			case Type::Array:
				return "std::Vector";
			case Type::Optional:
				return "::std::optional";
			case Type::UIDList:
				return "::serial::UIDList";
			default:
				return "UNKNOWN_type";
		}
	}

	constexpr std::ostream &operator<<(std::ostream &os, Type const &type) {
		switch (type) {
			case Type::UNKNOWN:
				return os << "serial::meta::Type.UNKNOWN";
			case Type::UNRESOLVED:
				return os << "serial::meta::Type.UNRESOLVED";
			case Type::U8:
				return os << "serial::meta::Type.U8";
			case Type::U16:
				return os << "serial::meta::Type.U16";
			case Type::U32:
				return os << "serial::meta::Type.U32";
			case Type::U64:
				return os << "serial::meta::Type.U64";
			case Type::I8:
				return os << "serial::meta::Type.I8";
			case Type::I16:
				return os << "serial::meta::Type.I16";
			case Type::I32:
				return os << "serial::meta::Type.I32";
			case Type::I64:
				return os << "serial::meta::Type.I64";
			case Type::Float:
				return os << "serial::meta::Type.Float";
			case Type::Double:
				return os << "serial::meta::Type.Double";
			case Type::Boolean:
				return os << "serial::meta::Type.Boolean";
			case Type::Enum:
				return os << "serial::meta::Type.Enum";
			case Type::Bitfield:
				return os << "serial::meta::Type.Bitfield";
			case Type::String:
				return os << "serial::meta::Type.String";
			case Type::Array:
				return os << "serial::meta::Type.Array";
			case Type::Optional:
				return os << "serial::meta::Type.Optional";
			case Type::UIDList:
				return os << "serial::meta::Type.UIDList";
			case Type::Struct:
				return os << "serial::meta::Type.Struct";
		}
	}

	util::Result<TypeSpec, Error> TypeSpec::create(
		cg::AstNode const &node,
		VersionScope const &scope
	) {
		if (node.cfg_rule() != "field-type") {
			return Error(ErrorType::INVALID_ARG, "Must pass field-type to TypeSpec::create");
		}

		auto t = TypeSpec();

		t._type = _get_type(node.tok());

		if (node.child_count() == 1) {
			if (t._type == Type::UNKNOWN || t._type == Type::UNRESOLVED) {
				return Error(ErrorType::VALIDATE_ERROR, std::format("Unrecognized primitive type {}", t._type));
			}
		} else if (node.child_count() == 4) {
			cg::AstNode *enclosed_node;
			if (auto err = node.child_with_cfg("field-type").move_or(enclosed_node)) {
				return Error(
					ErrorType::INVALID_STATE,
					"Expecting field-type node in generic type spec",
					err.value()
				);
			}
			auto enclosed = TypeSpec();
			if (auto err = TypeSpec::create(*enclosed_node, scope).move_or(enclosed)) {
				return Error(
					ErrorType::PARSE_ERROR,
					"Cannot parse enclosed generic type spec",
					err.value()
				);
			}

			t._enclosed = std::make_unique<TypeSpec>(std::move(enclosed));
		} else {
			return Error(ErrorType::PARSE_ERROR, "Expecting 1 or 4 child nodes");
		}

		return t;
	}

	Type TypeSpec::type() const {
		if (_type == Type::UNRESOLVED) {
			if (_scope->root().get_enum(_type_name, _scope->version())) {
				_type = Type::Enum;
			} else if (_scope->root().get_bitfield(_type_name, _scope->version())) {
				_type = Type::Bitfield;
			} else if (_scope->root().get_struct(_type_name, _scope->version())) {
				_type = Type::Struct;
			}
		}
		return _type;
	}

	Enum const *TypeSpec::get_enum() const {
		if (_scope == nullptr) {
			log_fatal_error() << "TypeSpec not initialized" << std::endl;
			return nullptr;
		}

		if (type() == Type::Enum) {
			return _scope->root().get_enum(_type_name, _scope->version());
		}
		return nullptr;
	}

	Bitfield const *TypeSpec::get_bitfield() const {
		if (_scope == nullptr) {
			log_fatal_error() << "TypeSpec not initialized" << std::endl;
			return nullptr;
		}

		if (type() == Type::Bitfield) {
			return _scope->root().get_bitfield(_type_name, _scope->version());
		}
		return nullptr;
	}

	Struct const *TypeSpec::get_struct() const {
		if (_scope == nullptr) {
			log_fatal_error() << "TypeSpec not initialized" << std::endl;
			return nullptr;
		}

		if (type() == Type::Struct) {
			return _scope->root().get_struct(_type_name, _scope->version());
		}
		return nullptr;
	}

	Container const *TypeSpec::get_container() const {
		if (_scope == nullptr) {
			log_fatal_error() << "TypeSpec not initialized" << std::endl;
			return nullptr;
		}

		auto t = type();
		if (t == Type::Optional || t == Type::Array || t == Type::UIDList) {
			return _scope->root().get_container(_type_name, _scope->version());
		}
		return nullptr;
	}

	util::Result<Field, Error> Field::create(
		cg::AstNode const &node,
		VersionScope const &scope,
		size_t idx
	) {
		log_assert(
			node.cfg_rule() == "struct-field",
			"Must pass pass property to TypeDef::create"
		);

		auto f = Field();
		cg::AstNode *spec_node;
		if (auto err = node.child_with_cfg("field-type").move_or(spec_node)) {
			return Error(
				ErrorType::INVALID_STATE, 
				"property doesn't have child node of type field-type",
				err.value()
			);
		}

		if (auto err = TypeSpec::create(*spec_node, scope).move_or(f._type_spec)) {
			return Error(
				ErrorType::PARSE_ERROR,
				"Couldn't parse property type",
				err.value()
			);
		}

		cg::AstNode *name_node;
		if (auto err = node.child_with_tok(int(TokenType::Identifier)).move_or(name_node)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Coulnd't find child with type identifier",
				err.value()
			);
		}
		f._name = name_node->consumed_all();
		f._idx = idx;

		return std::move(f);
	}

	std::string const &Field::name() const {
		return _name;
	}

	TypeSpec const &Field::spec() const {
		return _type_spec;
	}

	util::Result<Enumerator, Error> Enumerator::create(
		cg::AstNode const &node,
		Enum const &parent,
		VersionScope const &scope
	) {
		if (node.cfg_rule() != "enumerator") {
			return Error(
				ErrorType::INVALID_STATE,
				"Must pass enumerator AstNode to Enumerator::create"
			);
		}

		auto e = Enumerator();
		cg::AstNode const *child;
		if (auto err = node.child_with_tok(int(TokenType::Identifier)).move_or(child)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Expecting Identifier in enumerator",
				err.value()
			);
		}

		e._name = child->consumed_all();

		if (auto err = node.child_with_tok(int(TokenType::IntConst)).move_or(child)) {
			if (err->type() != cg::ErrorType::MISSING_AST_NODE) {
				return Error(
					ErrorType::INVALID_STATE,
					"Problem when looking up whether enumerator has value",
					err.value()
				);
			}
		} else {
			try {
				e._explicit_value = std::stoi(child->consumed_all());
			} catch (std::exception exc) {
				return Error(
					ErrorType::VALIDATE_ERROR,
					std::format("Couldn't parse enumerator int: {}", exc.what())
				);
			}
		}

		e._scope = &scope;
		e._parent = &parent;
		log_trace() << "Created enumerator " << e._name << std::endl;

		return e;
	}

	std::string const &Enumerator::name() const {
		return _name;
	}

	std::optional<uint64_t> Enumerator::explicit_value() const {
		return _explicit_value;
	}

	uint64_t Enumerator::implicit_value() const {
		log_error() << "TODOOOO" << std::endl;
		return 0;
	}

	util::Result<Enum, Error> Enum::create(
		cg::AstNode const &node,
		VersionScope const &scope
	) {
		if (node.cfg_rule() != "enum-decl") {
			return Error(
				ErrorType::INVALID_STATE,
				"Must pass enum-decl AstNode to Enum::create"
			);
		}

		auto e = Enum();
		cg::AstNode const *child;
		if (auto err = node.child_with_tok(int(TokenType::Identifier)).move_or(child)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Expecting Identifier in enum-decl",
				err.value()
			);
		}
		e._name = child->consumed_all();

		for (auto c : node.children_with_cfg("enumerator")) {
			Enumerator en;
			if (auto err = Enumerator::create(*c, e, scope).move_or(en)) {
				return Error(
					ErrorType::VALIDATE_ERROR,
					std::format("Couldn't validate enumerator of {}", e._name)
				);
			}
			e._enumerators.push_back(en);
		}

		e._scope = &scope;

		log_trace() << "Created enum " << e._name << std::endl;
		return e;
	}

	std::string const &Enum::name() const {
		return _name;
	}

	std::vector<Enumerator> const &Enum::enumerators() const {
		return _enumerators;
	}

	util::Result<BitfieldValue, Error> BitfieldValue::create(
		cg::AstNode const &node,
		uint8_t index,
		VersionScope const &scope
	) {
		if (node.cfg_rule() != "bitfield-field") {
			return Error(
				ErrorType::INVALID_STATE,
				"Must pass bitfield-field to BitfieldValue::create"
			);
		}
		if (index < 0) {
			return Error(
				ErrorType::INVALID_ARG,
				"BitfieldValue index must be >= 0"
			);
		}

		auto b = BitfieldValue();
		b._explict_idx = index;

		cg::AstNode const *child;
		if (auto err = node.child_with_tok(int(TokenType::Identifier)).move_or(child)) {
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

	std::string const &BitfieldValue::name() const {
		return _name;
	}

	util::Result<Bitfield, Error> Bitfield::create(
		cg::AstNode const &node,
		VersionScope const &scope
	) {
		if (node.cfg_rule() != "bitfield-decl") {
			return Error(
				ErrorType::INVALID_STATE,
				"Must pass bitfield-decl to Bitfield::create"
			);
		}

		auto b = Bitfield();

		cg::AstNode const *child;
		if (auto err = node.child_with_tok(int(TokenType::Identifier)).move_or(child)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Expecting Identifier in bitfield",
				err.value()
			);
		}
		b._name = child->consumed_all();

		auto i = 0;
		for (auto f : node.children_with_cfg("bitfield-field")) {
			BitfieldValue value;
			if (auto err = BitfieldValue::create(*f, i, scope).move_or(value)) {
				return Error(
					ErrorType::VALIDATE_ERROR,
					std::format("Could not validate field of bitfield {}", b._name)
				);
			}
			b._values.push_back(value);
			i++;
		}

		log_trace() << "Created bitfield " << b._name << std::endl;
		return b;
	}

	std::string const &Bitfield::name() const {
		return _name;
	}

	std::vector<BitfieldValue> const &Bitfield::values() const {
		return _values;
	}

	util::Result<Struct, Error> Struct::create(
		cg::AstNode const &node,
		VersionScope const &scope
	) {
		bool is_doc = node.cfg_rule() == "document-def";
		auto is_struct = node.cfg_rule() == "struct-def";
		if (!is_doc && !is_struct) {
			return Error(
				ErrorType::INVALID_STATE,
				"Struct::create must be passed document-ref or struct-def"
			);
		}

		auto s = Struct();
		s._is_document = is_doc;

		cg::AstNode const *name_node;
		if (auto err = node.child_with_tok(int(TokenType::Identifier)).move_or(name_node)) {
			return Error(
				ErrorType::PARSE_ERROR,
				"Expecting struct-def to have identifier child node"
			);
		}
		s._name = name_node->consumed_all();
		s._scope = &scope;

		size_t idx = 0;
		for (auto field_node : node.children_with_cfg("struct-field")) {
			auto field = Field();
			if (auto err = Field::create(*field_node, scope, idx++).move_or(field)) {
				return Error(
					ErrorType::PARSE_ERROR,
					"Couldn't parse struct field",
					err.value()
				);
			}
			s._fields.push_back(std::move(field));
		}
		return s;
	}

	std::string const &Struct::name() const {
		return _name;
	}

	std::vector<Field> const &Struct::fields() const {
		return _fields;
	}

	Field const *Struct::field(std::string const &name) const {
		for (auto &f : _fields) {
			if (f.name() == name) {
				return &f;
			}
		}
		return nullptr;
	}

	util::Result<Version, Error> Version::create(
		cg::AstNode const &node
	) {
		if (node.cfg_rule() != "version-frag") {
			return Error(
				ErrorType::INVALID_STATE,
				"Must pass version-frag to Version::create"
			);
		}

		auto v = Version();
		auto nums = node.children_with_tok(int(TokenType::IntConst));
		if (nums.size() != 3) {
			return Error(
				ErrorType::INVALID_STATE,
				"version-frag must contain 3 child IntConsts"
			);
		}

		try {
			v.major = std::stoi(nums[0]->consumed_all());
			v.minor = std::stoi(nums[0]->consumed_all());
			v.patch = std::stoi(nums[0]->consumed_all());
		} catch (std::exception const &e) {
			return Error(
				ErrorType::INVALID_STATE,
				std::format("Created version value {}.{}.{}", v.major, v.minor, v.patch)
			);
		}

		return v;
	}

	std::string Version::namespace_str() const {
		return std::format("{}.{}.{}", major, minor, patch);
	}

	bool Version::operator<(Version const &other) const {
		if (major != other.major) {
			return major < other.major;
		}
		if (minor != other.minor) {
			return minor < other.minor;
		}
		return patch < other.patch;
	}

	util::Result<VersionScope::Ptr, Error> VersionScope::create(
		cg::AstNode const &node,
		Root const &root
	) {
		Ptr scope = std::make_unique<VersionScope>();
		if (node.cfg_rule() != "version-decl") {
			return Error(
				ErrorType::INVALID_STATE,
				"Must pass version-decl AstNode to VersionScope::create"
			);
		}

		cg::AstNode const *value_node;
		if (auto err = node.child_with_cfg("version-frag").move_or(value_node)) {
			return Error(
				ErrorType::INVALID_STATE,
				"version-decl does not contain a version-frag child node",
				err.value()
			);
		}
		if (auto err = Version::create(*value_node).move_or(scope->_version)) {
			return Error(
				ErrorType::VALIDATE_ERROR,
				"Could not validate version fragment",
				err.value()
			);
		}

		cg::AstNode const *blck;
		if (auto err = node.child_with_cfg("version-blck").move_or(blck)) {
			return Error(
				ErrorType::INVALID_STATE,
				"version-decl does not contain a version-blck child node", 
				err.value()
			);
		}

		for (auto &child : *blck) {
			if (child.cfg_rule() == "enum-decl") {
				auto e = Enum();
				if (auto err = Enum::create(child, *scope).move_or(e)) {
					return Error(
						ErrorType::VALIDATE_ERROR,
						std::format("Could not validate enum in version {}", scope->version().namespace_str())
					);
				}
			}
		}
	}
}
