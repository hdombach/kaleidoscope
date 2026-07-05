#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <map>

#include "codegen/AstNode.hpp"
#include "codegen/TemplObj.hpp"
#include "util/result.hpp"
#include "Error.hpp"

/**
 * Takes in the AstNode tree and returns a TemplGen opposite for the codegen
 * Validates all the fields along the way
 */

namespace serial {
	using Node = cg::AstNode;
	using TemplObj = cg::TemplObj;
	class VVersion;

	class VEnumerator {
		public:
			VEnumerator() = default;
			static util::Result<VEnumerator, Error> create(Node const &node);

			TemplObj templ_obj() const;

			std::string const &name() const;
		private:
			std::string _name = "";
			std::optional<uint64_t> _value = std::nullopt;
	};

	class VEnum {
		public:
			VEnum() = default;
			static util::Result<VEnum, Error> create(Node const &node, std::string const &name);

			TemplObj templ_obj() const;

			std::string const &name() const;

			std::string const &filename() const;
		private:
			std::string _name;
			std::string _filename;
			std::vector<VEnumerator> _enumerators;
	};

	class VBFField {
		public:
			VBFField() = default;
			static util::Result<VBFField, Error> create(Node const &node, uint8_t index);

			TemplObj templ_obj() const;

			std::string const &name() const;
		private:
			std::string _name;
			uint8_t _index;
	};

	class VBitfield {
		public:
			VBitfield() = default;
			static util::Result<VBitfield, Error> create(Node const &node, std::string const &filename);

			TemplObj templ_obj() const;

			std::string const &name() const;

			std::string const &filename() const;
		private:
			std::string _name;
			std::string _filename;
			std::vector<VBFField> _fields;
	};

	/**
	 * @brief The type descrption that is provided for a struct field
	 *
	 * There are three types of types. Primitives, compound, and optional.
	 * Primitive types will trigger transactions that set the value
	 * Compound types will trigger transactions that modify an underlying type
	 * Optional types will have the same transactions as the enclosing type with
	 * an extra transaction added for setting to null.
	 */
	class VFieldType {
		public:
			VFieldType() = default;
			static util::Result<VFieldType, Error> create(Node const &node, VVersion &version);

			VFieldType(VFieldType const &other);
			VFieldType(VFieldType &&other) = default;
			VFieldType &operator=(VFieldType const &other);
			VFieldType &operator=(VFieldType &&other) = default;

			/**
			 * @brief Gets the cpp string representation
			 */
			std::string cpp_str() const;

			bool is_prim() const;
			bool is_opt() const;

		private:
			// Used to identify what type it is based off of tok type
			cg::Token const *_tok = nullptr;
			VVersion const *_version = nullptr;
			bool _is_opt;
			std::string _cpp_str_frag = "";
			std::unique_ptr<VFieldType> _enclosing_type = nullptr;

		private:

			bool _is_generic() const;
	};

	class VStructField {
		public:
			VStructField() = default;

			static util::Result<VStructField, Error> create(Node const &node, VVersion &version);

			TemplObj templ_obj() const;

			std::string const &name() const;

			VFieldType const &spec() const;
		private:
			std::string _name;
			VFieldType _spec;
	};

	class VStructDef {
		public:
			VStructDef() = default;
			static util::Result<VStructDef, Error> create(Node const &node, VVersion &version, std::string const &filename);

			TemplObj templ_obj() const;

			std::string const &name() const;

			std::string const &filename() const;

			std::map<std::string, VStructField> const &fields() const;
		private:
			VVersion *_version = nullptr;
			std::string _name;
			std::string _filename;
			std::map<std::string, VStructField> _fields;
	};

	struct VVersionValue {
		public:
			VVersionValue() = default;
			static util::Result<VVersionValue, Error> create(Node const &node);

			uint64_t vanity;
			uint64_t major;
			uint64_t minor;

			std::string namespace_str() const;

			bool operator<(VVersionValue const &other) const;
	};

	class VVersion {
		public:
			using Ptr = std::unique_ptr<VVersion>;

		public:
			VVersion() = default;

			static util::Result<Ptr, Error> create(Node const &node, std::string const &filename);

			TemplObj templ_obj(std::string const &filename) const;

			VVersionValue const &value() const;
			bool is_prim(std::string const &name) const;
		private:
			util::Result<void, Error> _check_identifier(
				std::string const &name,
				util::FileLocation floc=std::source_location::current()
			) const;

			VVersionValue _value;
			std::map<std::string, VEnum> _enums;
			std::map<std::string, VBitfield> _bitfields;
			std::map<std::string, VStructDef> _structs;
	};
	
	class VDocument {
		public:
			VDocument() = default;

			util::Result<void, Error> add_file(Node const &node, std::string const &filename);

			std::vector<std::string> const &includes() const;

			TemplObj templ_obj(std::string const &filename) const;

		private:
			std::vector<std::string> _includes;
			std::map<VVersionValue, VVersion::Ptr> _versions;
	};
}
