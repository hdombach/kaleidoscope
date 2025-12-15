#pragma once

#include <map>
#include <set>
#include <string>
#include <memory>

#include "util/result.hpp"
#include "codegen/CfgNode.hpp"
#include "Tokenizer.hpp"
#include "Error.hpp"

namespace cg {
	/**
	 * @brief Describes a Context Free Grammar
	 */
	class CfgContext {
		public:
			using Ptr = std::unique_ptr<CfgContext>;

			CfgContext() = default;

			static Ptr create();

			/**
			 * @brief Wrapper around CfgLeaf::token
			 */
			CfgLeaf t(Token::Type t) const;

			/**
			 * @brief Wrapper around CfgLeaf::empty
			 */
			CfgLeaf empty() const;

			/**
			 * @brief Wrapper around CfgLeaf::var
			 */
			CfgLeaf operator [](std::string const &str) const;

			/**
			 * @brief Creates a new primary node
			 *
			 * A primary node means the node will remain in the created
			 * abstract syntax tree after collapsing.
			 */
			CfgRuleSet &prim(std::string const &name);
			/**
			 * @brief Creates a new temporary node
			 *
			 * A temporary node means the node will automatically be collapsed
			 * in the created syntax tree.
			 */
			CfgRuleSet &temp(std::string const &name);
			/**
			 * @brief Defines the root rule for a grammar
			 */
			CfgRuleSet &root(std::string const &name);

			/**
			 * @brief Gets a node by name
			 * @returns Node or null if not found
			 */
			CfgRuleSet const *get(std::string const &name) const;

			CfgRuleSet const *get_root() const;

			std::vector<CfgRuleSet> const &cfg_rule_sets() const;
			uint32_t rule_id(std::string const &name) const;

			void debug_print(std::ostream &os) const;

			std::set<std::string> const &prim_names() const { return _prim_names; }

			/**
			 * @brief Validate and misc processing
			 * - Make sure variables are valid
			 * - Seperate all string leaf nodes
			 */
			util::Result<void, Error> prep();

			/**
			 * @brief Reduces the Context Free Grammar to only use basic operations
			 * - removes optional fields
			 * - Seperate all strings into individual character leaves
			 * - Create rules enumerating all possibilities in character sets
			 */
			void simplify();
		private:
			std::vector<CfgRuleSet> _cfg_rule_sets;
			std::map<std::string, uint32_t> _cfg_map;
			std::set<std::string> _prim_names;
			std::string _root_name;

			void _remove_empty();

			void _update_set_ids();
	};

}

inline std::ostream &operator<<(std::ostream &os, cg::CfgContext const &ctx) {
	ctx.debug_print(os);
	return os;
}
