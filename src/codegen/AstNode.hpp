#pragma once

#include <vector>
#include <set>

#include "util/FileLocation.hpp"
#include "util/result.hpp"
#include "Tokenizer.hpp"
#include "Error.hpp"

namespace cg {
	/**
	 * @brief Abstract syntax tree
	 *
	 * Can either correspond to a rule or a string literal.
	 * A rule will contain children corresponding to the CFG leaf nodes.
	 * A string literal contains the string that is consumed.
	 */

	class AstNodeIterator;
	class AstNode {
		public:
			enum class Type {
				None,
				Rule,
				Leaf
			};
		public:
			AstNode();

			AstNode(AstNode const &other);
			AstNode(AstNode &&other);

			AstNode &operator=(AstNode const &other);
			AstNode &operator=(AstNode &&other);
			/**
			 * @brief Create an astnode based on a rule
			 */
			static AstNode create_rule(
				uint32_t id,
				std::string const &cfg_name
			);
			/**
			 * @brief Create an astnode containg a string literal
			 */
			static AstNode create_tok(
				uint32_t id,
				Token const &token
			);

			Type type() const { return _type; }
			uint32_t id() const { return _id; }

			bool has_value() const;
			operator bool() const { return has_value(); }

			AstNodeIterator begin();
			AstNodeIterator end();
			AstNodeIterator begin() const;
			AstNodeIterator end() const;

			void add_child(AstNode &node);

			util::Result<AstNode*, Error> child_with_cfg(std::string const &name) const;

			std::vector<AstNode*> children_with_cfg(std::string const &name) const;

			util::Result<AstNode*, Error> child_with_tok(Token::Type type) const;

			/**
			 * @brief The rule that was used to generate this node
			 * Undefined behavior if this node is not of type rule
			 */
			std::string const &cfg_rule() const { return _cfg_rule; }

			Token const &tok() const;
			std::string consumed_all() const;

			/**
			 * @brief The number of leaf nodes
			 */
			size_t leaf_count() const;

			size_t child_count() const;

			util::FileLocation location() const;

			/**
			 * @brief Combines all nodes that aren't in the list of provided cfg's
			 */
			void compress(std::set<std::string> const &cfg_names);

			/**
			 * @brief Trims nodes that do not have children or consumed tokens
			 */
			void trim();
			/**
			 * @brief Trims node that do not have children or consumed tokens
			 */
			AstNode trimmed() const;

			std::ostream &print_debug(std::ostream &os) const;
			std::ostream &print_pre_order(std::ostream &os) const;
			std::ostream &print_src(std::ostream &os) const;
			std::ostream &print_dot(std::ostream &os, std::string const &name) const;

			std::string str() const;
			std::string str_src() const;
			std::string str_pre_order() const;

		private:
			friend class AstNodeIterator;
			Type _type;
			uint32_t _id;
			std::string _cfg_rule;
			Token const *_token=nullptr;

			// Use linked list to reduce number of reallocations.
			// All AstNodes are reserved in a pool in ParserContext to reduce fragmantation.
			AstNode *_sibling_prev=nullptr;
			AstNode *_sibling_next=nullptr;
			AstNode *_child_head=nullptr;
			AstNode *_child_tail=nullptr;

		private:
			void _print_dot_attributes(std::ostream &os) const;
			void _print_dot_paths(std::ostream &os) const;
	};
}

inline std::ostream &operator<<(std::ostream &os, cg::AstNode const &node) {
	return node.print_debug(os);
}

