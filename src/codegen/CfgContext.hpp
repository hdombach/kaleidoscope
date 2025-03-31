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
			 * @brief Gets a node by name
			 * @returns Node or null if not found
			 */
			CfgRuleSet const *get(std::string const &name) const;

			void debug_set(CfgRuleSet const &set, std::ostream &os) const;
			void debug_set(std::string const &set, std::ostream &os) const;
			std::string set_str(CfgRuleSet const &set) const;
			std::string set_str(std::string const &name) const;

			std::set<std::string> const &prim_names() const { return _prim_names; }

			/**
			 * @brief Validate and misc processing
			 * - Make sure variables are valid
			 * - Seperate all string leaf nodes
			 */
			util::Result<void, KError> prep();
		private:
			std::map<std::string, CfgRuleSet> _cfg_map;
			std::set<std::string> _prim_names;
	};
}
