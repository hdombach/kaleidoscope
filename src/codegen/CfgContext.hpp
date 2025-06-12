#pragma once

#include <map>
#include <set>
#include <string>

#include "util/result.hpp"
#include "util/KError.hpp"
#include "codegen/CfgNode.hpp"

namespace cg {
	/**
	 * @brief Describes a Context Free Grammar
	 */
	class CfgContext {
		public:
			CfgContext() = default;

			/**
			 * @brief Wrapper around CfgLeaf::str
			 */
			CfgLeaf s(std::string const &str) const;

			/**
			 * @brief Wrapper around CfgLeaf::include
			 */
			CfgLeaf i(std::string const &str) const;

			/**
			 * @brief Wrapper around CfgLeaf::exclude
			 */
			CfgLeaf e(std::string const &str) const;

			/**
			 * @brief Wrapper around CfgLeaf::character
			 */
			CfgLeaf c(char c) const;

			/**
			 * @brief Creates an end of character
			 */
			CfgLeaf eof() const;

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

			void debug_set(CfgRuleSet const &set, std::ostream &os) const;
			void debug_set(std::string const &set, std::ostream &os) const;
			void debug_sets(std::ostream &os) const;
			std::string set_str(CfgRuleSet const &set) const;
			std::string set_str(std::string const &name) const;

			std::set<std::string> const &prim_names() const { return _prim_names; }

			/**
			 * @brief Validate and misc processing
			 * - Make sure variables are valid
			 * - Seperate all string leaf nodes
			 */
			util::Result<void, KError> prep();

			/**
			 * @brief Reduces the Context Free Grammar to only use basic operations
			 * - Seperate all strings into individual character leaves
			 * - Create rules enumerating all possibilities in character sets
			 */
			void simplify();
		private:
			std::vector<CfgRuleSet> _cfg_rule_sets;
			std::map<std::string, uint32_t> _cfg_map;
			std::set<std::string> _prim_names;
			std::string _root_name;
	};

	inline std::ostream &operator<<(std::ostream &os, CfgContext const &ctx) {
		ctx.debug_sets(os);
		return os;
	}
}
