#pragma once

#include <string>
#include <vector>
#include <format>

#include "codegen/TemplObj.hpp"
#include "codegen/AstNode.hpp"
#include "Error.hpp"

namespace serial::meta {
	class Root;
	class Enum;
	class Bitfield;
	class Struct;
	class VersionScope;

	/**
	 * @brief Specifies a field type
	 */
	enum class Type {
		UNKNOWN = 0,
		UNRESOLVED,
		U8,
		U16,
		U32,
		U64,
		I8,
		I16,
		I32,
		I64,
		Float,
		Double,
		Boolean,
		Enum,
		Bitfield,
		String,
		Array,
		Optional,
		UIDList,
		Struct,
	};

	constexpr std::ostream &operator<<(std::ostream &os, Type const &type);

	static_assert(OsPrintable<serial::meta::Type >);

	class TypeSpec {
		public:
			TypeSpec() = default;
			static util::Result<TypeSpec, Error> create(
				cg::AstNode const &node,
				VersionScope const &scope
			);

			Type type() const;

			Enum const *get_enum() const;
			Bitfield const *get_bitfield() const;
			Struct const *get_struct() const;

		private:
			mutable Type _type = Type::UNKNOWN;
			std::unique_ptr<TypeSpec> _enclosed = nullptr;
			VersionScope const *_scope = nullptr;
			std::string _type_name;
	};

	class Field {
		public:
			Field() = default;
			static util::Result<Field, Error> create(
				cg::AstNode const &node,
				VersionScope const &scope,
				size_t idx
			);

			std::string const &name() const;

			TypeSpec const &spec() const;

		private:
			TypeSpec _type_spec;
			std::string _name;
			size_t _idx;
			VersionScope const *_scope = nullptr;
	};

	class Enumerator {
		public:
			Enumerator() = default;
			static util::Result<Enumerator, Error> create(
				cg::AstNode const &node,
				Enum const &parent,
				VersionScope const &scope
			);

			std::string const &name() const;

			std::optional<uint64_t> explicit_value() const;
			uint64_t implicit_value() const;

		private:
			VersionScope const *_scope = nullptr;
			Enum const *_parent = nullptr;
			std::string _name = "";
			std::optional<uint64_t> _explicit_value = std::nullopt;
	};

	class Enum {
		public:
			Enum() = default;
			static util::Result<Enum, Error> create(
				cg::AstNode const &node,
				VersionScope const &scope
			);

			std::string const &name() const;

			std::vector<Enumerator> const &enumerators() const;
		private:
			std::string _name;
			std::vector<Enumerator> _enumerators;
			VersionScope const *_scope = nullptr;
	};

	class BitfieldValue {
		public:
			BitfieldValue() = default;
			static util::Result<BitfieldValue, Error> create(
				cg::AstNode const &node,
				uint8_t index,
				VersionScope const &scope
			);

			std::string const &name() const;

			std::optional<uint64_t> explicit_idx() const;
			uint64_t implicit_idx() const;

		private:
			VersionScope const *_scope;
			Bitfield const *_parent;
			std::string _name;
			std::optional<int64_t> _explict_idx;
	};

	class Bitfield {
		public:
			Bitfield() = default;
			static util::Result<Bitfield, Error> create(
				cg::AstNode const &node,
				VersionScope const &scope
			);

			std::string const &name() const;

			std::vector<BitfieldValue> const &values() const;

		private:
			std::string _name;
			std::vector<BitfieldValue> _values;
	};

	class Struct {
		public:
			Struct() = default;
			static util::Result<Struct, Error> create(
				cg::AstNode const &node,
				VersionScope const &scope
			);

			std::string const &name() const;

			std::vector<Field> const &fields() const;

			Field const *field(std::string const &name) const;

		private:
			bool _is_document;
			std::string _name;
			std::vector<Field> _fields;
			VersionScope const *_scope;
	};

	struct Version {
		public:
			Version() = default;
			static util::Result<Version, Error> create(
				cg::AstNode const &node
			);

			uint64_t major;
			uint64_t minor;
			uint64_t patch;

			std::string namespace_str() const;

			bool operator<(Version const &other) const;
	};

	class VersionScope {
		public:
			using Ptr = std::unique_ptr<VersionScope>;
		public:
			VersionScope() = default;
			static util::Result<VersionScope::Ptr, Error> create(
				cg::AstNode const &node,
				Root const &root
			);

			Version const &version() const;

			std::vector<Enum> const enums() const;
			std::vector<Bitfield> const &bitfields() const;
			std::vector<Struct> const &structs() const;

			Root const &root() const;

			Enum const *get_enum(std::string const &name) const;
			Bitfield const *get_bitfield(std::string const &name) const;
			Struct const *get_struct(std::string const &name) const;

		private:
			Version _version;
	};

	class File {
		public:
			File() = default;
			static util::Result<File, Error> create(
				cg::AstNode const &node,
				Root const &root
			);

			std::vector<std::string> const &includes() const;

			std::vector<VersionScope> const &versions() const;
	};

	class Root {
		public:
			using Ptr = std::unique_ptr<Root>;
		public:
			Root() = default;
			static Ptr create();

			util::Result<void, Error> add_file(
				cg::AstNode const &node,
				std::string const &file
			);

			std::vector<File> const &files() const;

			File const *file(std::string const &filename) const;

			Enum const *get_enum(std::string const &name, Version const &version) const;
			Bitfield const *get_bitfield(std::string const &name, Version const &version) const;
			Struct const *get_struct(std::string const &name, Version const &version) const;
	};

	cg::TemplObj templ_obj(TypeSpec const &type_spec);
	cg::TemplObj templ_obj(Field const &field);
	cg::TemplObj templ_obj(Enumerator const &enumerator);
	cg::TemplObj templ_obj(Enum const &enum_v);
	cg::TemplObj templ_obj(BitfieldValue const &bfield_value);
	cg::TemplObj templ_obj(Bitfield const &bfield);
	cg::TemplObj templ_obj(Struct const &s);

	std::string namespace_str(Version const &v);
}

template<>
struct std::formatter<serial::meta::Type, char>:
	public DefaultFormatter<serial::meta::Type>
{};
