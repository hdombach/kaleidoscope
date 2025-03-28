#pragma once

#include <map>
#include <set>
#include <string>

#include "util/UIDList.hpp"
#include "util/result.hpp"
#include "util/KError.hpp"
#include "codegen/CfgNode.hpp"

namespace cg {
	class CfgContextTemp {
		public:
			CfgContextTemp() = default;

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
		private:
			std::map<std::string, CfgRuleSet> _cfg_map;
			std::set<std::string> _prim_names;
	};

	/**
	 * @brief Helps build and store CfgNodes
	 */

	class CfgContext {
		public:
			using Type = CfgNode::Type;
		public:
			CfgContext() = default;

			/******************************
			 * Node builder methods 
			 ******************************/

			/**
			 * @brief Wrapper around CfgNode::literal
			 */
			CfgNode lit(std::string const &str) const;

			/**
			 * @brief Creates a reference grammar object
			 *
			 * Is a wrapper around CfgNode::ref with a proper lookup
			 * for the referenced name
			 */
			CfgNode ref(std::string const &name) const;

			/**
			 * @brief Wrapper around CfgNode::seq
			 */
			CfgNode seq(CfgNode &&lhs, CfgNode &&rhs) const;

			/**
			 * @brief Wrapper around CfgNode::alt
			 */
			CfgNode alt(CfgNode &&lhs, CfgNode &&rhs) const;

			/**
			 * @brief Wrapper round CfgNode::cls
			 */
			CfgNode cls(CfgNode &&c) const;

			/**
			 * @brief Wrapper around CfgNode::opt
			 */
			CfgNode opt(CfgNode &&c) const;

			/**
			 * @brief Wrapper around CfgNode::neg
			 */
			CfgNode neg(CfgNode &&c) const;

			/**
			 * @brief Duplicates a node
			 */
			CfgNode dup(std::string const &name) const;

			/**
			 * @brief Resolves internal nodes
			 */
			util::Result<void, KError> prep();

			bool contains(std::string const &name) const;

			bool contains(uint32_t id) const;

			/**
			 * @brief Creates a new primary node
			 */
			CfgNode &prim(std::string const &name);
			/**
			 * @brief Creates a new temporary node
			 */
			CfgNode &temp(std::string const &name);

			/**
			 * @brief Gets node by name
			 * @returns Node or null if not found
			 */
			CfgNode const *get(std::string const &name) const;

			/**
			 * @brief Gets node by id
			 */
			CfgNode const &get(uint32_t id) const;

			/**
			 * @brief Create reference of string
			 */
			CfgNode operator[](std::string const &name) const { return ref(name); }

			void debug_node(CfgNode const &node, std::ostream &os) const;
			void debug_node(std::string const &name, std::ostream &os) const;
			std::string node_str(CfgNode const &name) const;
			std::string node_str(std::string const &name) const;

			/** @brief Names of primary nodes you compress down to) */
			std::vector<std::string> const &prim_names() const { return _prim_names; }

		private:
			std::map<std::string, uint32_t> _cfg_map;
			util::UIDList<CfgNode> _cfgs;
			std::vector<std::string> _prim_names;
	};

	inline CfgNode operator+(CfgNode &&lhs, CfgNode &&rhs) {
		return CfgNode::seq(std::move(lhs), std::move(rhs));
	}
	inline CfgNode operator|(CfgNode &&lhs, CfgNode &&rhs) {
		return CfgNode::alt(std::move(lhs), std::move(rhs));
	}
	inline CfgNode operator!(CfgNode &&node) {
		return CfgNode::neg(std::move(node));
	}
}
