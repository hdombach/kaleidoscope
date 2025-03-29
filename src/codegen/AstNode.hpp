#pragma once

#include <vector>
#include <set>

#include "util/FileLocation.hpp"
#include "util/KError.hpp"
#include "util/result.hpp"

namespace cg {
	/**
	 * @brief Abstract syntax tree
	 * The type is specified by the cfg;
	 */

	class AstNode {
		public:
			enum class Type {
				None,
				Rule,
				String
			};
		public:
			AstNode();
			/**
			 * @brief Create an astnode based on a rule
			 */
			static AstNode create_rule(
				uint32_t id,
				std::string const &cfg_name,
				util::FileLocation const &file_location
			);
			/**
			 * @brief Create an astnode containg a string literal
			 */
			static AstNode create_str(
				uint32_t id,
				std::string const &str,
				util::FileLocation const &file_location
			);

			Type type() const { return _type; }
			uint32_t id() const { return _id; }

			bool has_value() const;
			operator bool() const { return has_value(); }

			std::vector<AstNode> const &children() const { return _children; }

			void add_child(AstNode const &node);

			util::Result<AstNode, KError> child_with_cfg(std::string const &name) const;

			std::vector<AstNode> children_with_cfg(std::string const &name) const;

			/**
			 * @brief The rule that was used to generate this node
			 * Undefined behavior if this node is not of type rule
			 */
			std::string const &cfg_rule() const { return _cfg_rule; }

			std::string const &consumed() const { return _consumed; }

			/**
			 * @brief Number of characters by this node and any children node
			 */
			size_t size() const;

			util::FileLocation location() const;

			/**
			 * @brief Combines all nodes that aren't in the list of provided cfg's
			 */
			void compress(std::set<std::string> const &cfg_names);

			AstNode compressed(
				std::set<std::string> const &cfg_names
			) const;

			/**
			 * @brief Trims nodes that do not have children or consumed characters
			 */
			void trim();
			/**
			 * @brief Trims node that do not have children or consumed characters
			 */
			AstNode trimmed() const;

			std::ostream &print_pre_order(std::ostream &os) const;
			std::ostream &print_dot(std::ostream &os) const;

			std::string str_pre_order() const;

		private:
			Type _type;
			uint32_t _id;
			std::string _cfg_rule;
			std::string _consumed;
			util::FileLocation _location;
			std::vector<AstNode> _children;

			/**
			 * @brief The cached size
			 */
			mutable size_t _size;

		private:
			size_t _calc_size() const;
			void _print_dot_attributes(std::ostream &os) const;
			void _print_dot_paths(std::ostream &os) const;
	};
}
