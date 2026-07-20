#include "Validate.hpp"

#include "Tokenizer.hpp"
#include "codegen/AstNode.hpp"
#include "codegen/AstNodeIterator.hpp"
#include "codegen/TemplObj.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include <cstddef>

namespace serial {
	using T = TokenType;
	util::Result<VEnumerator, Error> VEnumerator::create(const Node &node) {
		log_assert(
			node.cfg_rule() == "enumerator",
			"Must pass enumerator AstNode to Enumerator::create"
		);
		
		auto e = VEnumerator();
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

	TemplObj VEnumerator::templ_obj() const {
		return {
			{"name", _name},
			{"value", cg::TemplInt(_value.value_or(0))},
			{"has_value", _value.has_value()}
		};
	}

	std::string const &VEnumerator::name() const {
		return _name;
	}

	util::Result<VEnum, Error> VEnum::create(Node const &node, std::string const &filename) {
		log_assert(
			node.cfg_rule() == "enum-decl",
			"Must pass enum-decl AstNode to Enum::create"
		);

		auto e = VEnum();
		Node const *child;
		if (auto err =  node.child_with_tok(int(T::Identifier)).move_or(child)) {
			return Error(ErrorType::INVALID_STATE, "Expecting Identifier in enum-decl", err.value());
		}
		e._name = child->consumed_all();
		e._filename = filename;

		for (auto c : node.children_with_cfg("enumerator")) {
			VEnumerator en;
			if (auto err = VEnumerator::create(*c).move_or(en)) {
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

	TemplObj VEnum::templ_obj() const {
		auto enumerators = cg::TemplList();

		for (auto &e : _enumerators) {
			enumerators.push_back(e.templ_obj());
		}

		return {
			{"name", _name},
			{"enumerators", enumerators}
		};
	}

	std::string const &VEnum::name() const {
		return _name;
	}

	std::string const &VEnum::filename() const {
		return _filename;
	}

	util::Result<VBFField, Error> VBFField::create(Node const &node, uint8_t index) {
		log_assert(
			node.cfg_rule() == "bitfield-field",
			"Must pass bitfield-field to BFField::create"
		);
		log_assert(index > 0, "BFField index must be greater than 0");

		auto b = VBFField();
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

	TemplObj VBFField::templ_obj() const {
		return {
			{"name", _name},
			{"index", _index}
		};
	}

	std::string const &VBFField::name() const {
		return _name;
	}

	util::Result<VBitfield, Error> VBitfield::create(Node const &node, std::string const &filename) {
		log_assert(
			node.cfg_rule() == "bitfield-decl",
			"Must pass bitfield-decl to Bitfield::create"
		);

		auto b = VBitfield();

		Node const *child;
		if (auto err = node.child_with_tok(int(T::Identifier)).move_or(child)) {
			return Error(
				ErrorType::INVALID_STATE,
				"Expecting Identifier in bitfield",
				err.value()
			);
		}
		b._name = child->consumed_all();
		b._filename = filename;

		auto i = 1;
		for (auto f : node.children_with_cfg("bitfield-field")) {
			VBFField field;
			if (auto err = VBFField::create(*f, i).move_or(field)) {
				return Error(ErrorType::VALIDATE_ERROR, util::f("Could not validate field of bitfield ", b._name), err.value());
			}
			b._fields.push_back(field);
			i++;
		}

		log_trace() << "Created bitfield " << b._name << std::endl;
		return b;
	}

	TemplObj VBitfield::templ_obj() const {
		auto fields = cg::TemplList();
		for (auto &f : _fields) {
			fields.push_back(f.templ_obj());
		}

		return {
			{"name", _name},
			{"fields", fields}
		};
	}

	std::string const &VBitfield::name() const {
		return _name;
	}

	std::string const &VBitfield::filename() const {
		return _filename;
	}

	const char *_get_cpp_str_frag(cg::Token const &tok) {
		switch (T(tok.type())) {
			case T::Float:
				return "float";
			case T::Double:
				return "double";
			case T::U8:
				return "uint8_t";
			case T::U16:
				return "uint16_t";
			case T::U32:
				return "uint32_t";
			case T::U64:
				return "uint64_t";
			case T::I8:
				return "int8_t";
			case T::I16:
				return "int16_t";
			case T::I32:
				return "int32_t";
			case T::I64:
				return "int64_t";
			case T::String:
				return "::std::string";
			case T::Array:
				return "::serial::Vector";
			case T::Optional:
				return "::std::optional";
			case T::UIDList:
				return "::serial::UIDList";
			case T::Identifier:
				return tok.content().c_str();
			default:
				return "UNKNOWN";
		}
	}

	util::Result<VFieldType, Error> VFieldType::create(Node const &node, VVersion &version) {
		log_assert(node.cfg_rule() == "field-type", "Must pass field-type to TypeSpec::create");

		auto t = VFieldType();

		t._is_opt = T(node.begin()->tok().type()) == T::Optional;
		t._tok = &node.begin()->tok();
		t._version = &version;

		if (node.child_count() == 1) {
			t._cpp_str_frag = _get_cpp_str_frag(node.begin()->tok());
		} else if (node.child_count() == 4) {
			t._cpp_str_frag = _get_cpp_str_frag(node.begin()->tok());
			Node *enclosed_node;
			if (auto err = node.child_with_cfg("field-type").move_or(enclosed_node)) {
				return Error(ErrorType::PARSE_ERROR, "Expecting field-type node in generic type spec", err.value());
			}
			auto enclosed = VFieldType();
			if (auto err = VFieldType::create(*enclosed_node, version).move_or(enclosed)) {
				return Error(ErrorType::PARSE_ERROR, "Cannot parse enclosed generic type spec", err.value());
			}
			t._enclosing_type = std::make_unique<VFieldType>(std::move(enclosed));
		} else {
			return Error(ErrorType::PARSE_ERROR, "Expecting 1 or four child nodes");
		}

		if (t._is_opt) {
			log_assert(t._enclosing_type.get(), "Optional type must contain an enclosing type");
			if (t._enclosing_type->is_opt()) {
				return Error(ErrorType::PARSE_ERROR, "Cannot nest optional.", node.location());
			}
		}

		return t;
	}

	VFieldType::VFieldType(VFieldType const &other) {
		_version = other._version;
		_tok = other._tok;
		_cpp_str_frag = other._cpp_str_frag;
		_is_opt = other._is_opt;
		if (other._enclosing_type) {
			_enclosing_type = std::make_unique<VFieldType>(*other._enclosing_type);
		}
	}

	VFieldType &VFieldType::operator=(VFieldType const &other) {
		_version = other._version;
		_tok = other._tok;
		_cpp_str_frag = other._cpp_str_frag;
		_is_opt = other._is_opt;
		if (other._enclosing_type) {
			_enclosing_type = std::make_unique<VFieldType>(*other._enclosing_type);
		}
		return *this;
	}

	std::string VFieldType::cpp_str() const {
		if (_is_generic()) {
			return util::f(_cpp_str_frag, "<", _enclosing_type->cpp_str(), ">");
		} else {
			return _cpp_str_frag;
		}
	}

	bool VFieldType::is_prim() const {
		log_assert(_tok, "_tok must be set");
		log_assert(_version, "_version must be set");
		switch (T(_tok->type())) {
			case T::Float:
			case T::Double:
			case T::U8:
			case T::U16:
			case T::U32:
			case T::U64:
			case T::I8:
			case T::I16:
			case T::I32:
			case T::I64:
			case T::String:
				return true;
			case T::Optional:
				log_assert(_enclosing_type.get(), "Enclosing type must be set for optional type");
				return _enclosing_type->is_prim();
			case T::Array:
			case T::UIDList:
				return false;
			case T::Identifier:
				return _version->is_prim(_tok->content());
			default:
				log_warning() << "Unrecognized token: " << _tok->type() << std::endl;
				return false;
		}	}

	bool VFieldType::is_opt() const {
		return _is_opt;
	}

	bool VFieldType::_is_generic() const {
		return _enclosing_type.get() != nullptr;
	}

	util::Result<VStructField, Error> VStructField::create(Node const &node, VVersion &version) {
		log_assert(node.cfg_rule() == "struct-field", "Must pass property to TypeDef::create");

		auto f = VStructField();
		Node *spec_node;
		if (auto err = node.child_with_cfg("field-type").move_or(spec_node)) {
			return Error(ErrorType::PARSE_ERROR, "property doesn't have child node of type field-type", err.value());
		}
		if (auto err = VFieldType::create(*spec_node, version).move_or(f._spec)) {
			return Error(ErrorType::PARSE_ERROR, "Couldn't parse property type", err.value());
		}

		Node *name_node;
		if (auto err = node.child_with_tok(int(T::Identifier)).move_or(name_node)) {
			return Error(ErrorType::PARSE_ERROR, "Couldn't find child with type identifier", err.value());
		}
		f._name = name_node->consumed_all();

		return std::move(f);
	}

	TemplObj VStructField::templ_obj() const {
		return {
			{"type_str", _spec.cpp_str()},
			{"is_primitive", _spec.is_prim()},
			{"is_optional", _spec.is_opt()},
			{"name", _name},
			{"set_trans_name", util::f("TSet_", _name)},
			{"mod_trans_name", util::f("TModify_", _name)},
		};
	}

	std::string const &VStructField::name() const { return _name; }

	VFieldType const &VStructField::spec() const { return _spec; }

	util::Result<VStructDef, Error> VStructDef::create(Node const &node, VVersion &version, std::string const &filename) {
		log_assert(node.cfg_rule() == "struct-def", "Must pass struct-def to StructDef::create");

		auto s = VStructDef();

		Node *name_node;
		if (auto err = node.child_with_tok(int(T::Identifier)).move_or(name_node)) {
			return Error(ErrorType::PARSE_ERROR, "Expecting struct-def to have identifier child node");
		}
		s._name = name_node->consumed_all();
		s._filename = filename;
		s._version = &version;

		for (auto field_node : node.children_with_cfg("struct-field")) {
			auto field = VStructField();
			if (auto err = VStructField::create(*field_node, version).move_or(field)) {
				return Error(ErrorType::PARSE_ERROR, "Couldn't parse struct field", err.value());
			}
			s._fields[field.name()] = std::move(field);
		}

		return s;
	}

	TemplObj VStructDef::templ_obj() const {
		auto fields = cg::TemplList();
		for (auto &[name, f] : _fields) {
			fields.push_back(f.templ_obj());
		}
		log_assert(!_name.empty(), "StructDef must be setup before calling templ_obj");
		return {
			{"name", _name},
			{"fields", fields}
		};
	}

	std::string const &VStructDef::name() const {
		return _name;
	}

	std::string const &VStructDef::filename() const {
		return _filename;
	}

	util::Result<VVersionValue, Error> VVersionValue::create(Node const &node) {
		log_assert(node.cfg_rule() == "version-frag", "Must pass version-frag to VersionValue::create");

		auto v = VVersionValue();
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

	std::string VVersionValue::namespace_str() const {
		return util::f("v", vanity, "_", major, "_", minor);
	}

	bool VVersionValue::operator<(VVersionValue const &other) const {
		if (vanity != other.vanity) {
			return vanity < other.vanity;
		}
		if (major != other.major) {
			return major < other.major;
		}
		return minor < other.minor;
	}

	util::Result<VVersion::Ptr, Error> VVersion::create(Node const &node, std::string const &filename) {
		Ptr v = std::make_unique<VVersion>();
		log_assert(node.cfg_rule() == "version-decl", "Must pass version-decl AstNode to Version::create");

		Node *value_node;
		if (auto err = node.child_with_cfg("version-frag").move_or(value_node)) {
			return Error(ErrorType::INVALID_STATE, "version-decl does not contain a version-frag child node", err.value());
		}
		if (auto err = VVersionValue::create(*value_node).move_or(v->_value)) {
			return Error(ErrorType::VALIDATE_ERROR, "Could not validate version fragment", err.value());
		}

		Node *blck;
		if (auto err = node.child_with_cfg("version-blck").move_or(blck)) {
			return Error(ErrorType::INVALID_STATE, "version-decl does not contain a version-blck child node", err.value());
		}

		for (auto &child : *blck) {
			if (child.cfg_rule() == "enum-decl") {
				auto e = VEnum();
				if (auto err = VEnum::create(child, filename).move_or(e)) {
					return Error(ErrorType::VALIDATE_ERROR, util::f("Could not validate enum in version ", v->_value.namespace_str()), err.value());
				}
				if (auto err = v->_check_identifier(e.name()).move_or()) {
					return err.value();
				}
				v->_enums[e.name()] = e;
			} else if (child.cfg_rule() == "bitfield-decl") {
				auto b = VBitfield();
				if (auto err = VBitfield::create(child, filename).move_or(b)) {
					return Error(ErrorType::VALIDATE_ERROR, util::f("Could not validate bitfield in version ", v->_value.namespace_str()), err.value());
				}
				if (auto err = v->_check_identifier(b.name()).move_or()) {
					return err.value();
				}
				v->_bitfields[b.name()] = b;
			} else if (child.cfg_rule() == "struct-def") {
				auto s = VStructDef();
				if (auto err = VStructDef::create(child, *v, filename).move_or(s)) {
					return Error(ErrorType::VALIDATE_ERROR, util::f("Could not validate struct-def in version ", v->_value.namespace_str()), err.value());
				}
				v->_structs[s.name()] = s;
			}
		}

		return {std::move(v)};
	}

	TemplObj VVersion::templ_obj(std::string const &filename) const {
		auto enums = cg::TemplList();
		for (auto &[name, e] : _enums) {
			if (e.filename() != filename) continue;
			enums.push_back(e.templ_obj());
		}

		auto bitfields = cg::TemplList();
		for (auto &[name, b] : _bitfields) {
			if (b.filename() != filename) continue;
			bitfields.push_back(b.templ_obj());
		}

		auto structs = cg::TemplList();
		for (auto &[name, s] : _structs) {
			if (s.filename() != filename) continue;
			structs.push_back(s.templ_obj());
		}

		return {
			{"namespace_str", _value.namespace_str()},
			{"enums", enums},
			{"bitfields", bitfields},
			{"structs", structs},
		};
	}

	VVersionValue const &VVersion::value() const {
		return _value;
	}

	bool VVersion::is_prim(std::string const &name) const {
		return _enums.contains(name) || _bitfields.contains(name);
	}

	util::Result<void, Error> VVersion::_check_identifier(
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

	util::Result<void, Error> VDocument::add_file(Node const &node, std::string const &filename) {
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
				VVersion::Ptr v;
				if (auto err = VVersion::create(child, filename).move_or(v)) {
					return Error(ErrorType::PARSE_ERROR, "Could not parse version", err.value());
				}
				_versions[v->value()] = std::move(v);
			}
		}
		return {};
	}

	std::vector<std::string> const &VDocument::includes() const {
		return _includes;
	}

	TemplObj VDocument::templ_obj(std::string const &filename) const {
		auto versions = cg::TemplList();

		auto filepath = std::filesystem::path(filename);
		auto source_path = util::f(filepath.stem().c_str(), ".cpp");
		auto header_path = util::f(filepath.stem().c_str(), ".hpp");

		for (auto &[name, v] : _versions) {
			versions.push_back(v->templ_obj(filename));
		}
		return {
			{"versions", versions},
			{"source_path", source_path},
			{"header_path", header_path},
		};
	}
}
