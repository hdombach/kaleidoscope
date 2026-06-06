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

	class Enumerator {
		public:
			Enumerator() = default;
			static util::Result<Enumerator, Error> create(Node const &node);

			TemplObj templ_obj() const;

			std::string const &name() const;
		private:
			std::string _name = "";
			std::optional<uint64_t> _value = std::nullopt;
	};

	class Enum {
		public:
			Enum() = default;
			static util::Result<Enum, Error> create(Node const &node);

			TemplObj templ_obj() const;

			std::string const &name() const;
		private:
			std::string _name;
			std::vector<Enumerator> _enumerators;
	};

	class BFField {
		public:
			BFField() = default;
			static util::Result<BFField, Error> create(Node const &node, uint8_t index);

			TemplObj templ_obj() const;

			std::string const &name() const;
		private:
			std::string _name;
			uint8_t _index;
	};

	class Bitfield {
		public:
			Bitfield() = default;
			static util::Result<Bitfield, Error> create(Node const &node);

			TemplObj templ_obj() const;

			std::string const &name() const;
		private:
			std::string _name;
			std::vector<BFField> _fields;
	};

	struct VersionValue {
		public:
			VersionValue() = default;
			static util::Result<VersionValue, Error> create(Node const &node);

			uint64_t vanity;
			uint64_t major;
			uint64_t minor;

			std::string namespace_str() const;

			bool operator<(VersionValue const &other) const;
	};

	class Version {
		public:
			Version() = default;

			static util::Result<Version, Error> create(Node const &node);

			TemplObj templ_obj() const;

			VersionValue const &value() const;
		private:
			util::Result<void, Error> _check_identifier(
				std::string const &name,
				util::FileLocation floc=std::source_location::current()
			) const;

			VersionValue _value;
			std::map<std::string, Enum> _enums;
			std::map<std::string, Bitfield> _bitfields;
	};
	
	class Document {
		public:
			Document() = default;

			util::Result<void, Error> add_file(Node const &node);

			std::vector<std::string> const &includes() const;

			TemplObj templ_obj() const;

		private:
			std::vector<std::string> _includes;
			std::map<VersionValue, Version> _versions;
	};
}
