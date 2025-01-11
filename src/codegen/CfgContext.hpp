#pragma once

#include <map>
#include <string>

#include "util/UIDList.hpp"
#include "util/result.hpp"
#include "util/errors.hpp"
#include "codegen/CfgNode.hpp"

namespace cg {
	/**
	 * @brief Helps build and store CfgNodes
	 */

	class CfgContext {
		public:
			CfgContext() = default;

			/******************************
			 * Node builder methods 
			 ******************************/

			/**
			 * @brief Wrapper around CfgNode::literal
			 */
			CfgNode lit(std::string const &str);

			/**
			 * @brief Creates a reference grammar object
			 *
			 * Is a wrapper around CfgNode::ref with a proper lookup
			 * for the referenced name
			 */
			CfgNode ref(std::string const &name);

			/**
			 * @brief Wrapper around CfgNode::seq
			 */
			CfgNode seq(CfgNode &&lhs, CfgNode &&rhs);

			/**
			 * @brief Wrapper around CfgNode::alt
			 */
			CfgNode alt(CfgNode &&lhs, CfgNode &&rhs);

			/**
			 * @brief Wrapper round CfgNode::cls
			 */
			CfgNode cls(CfgNode &&c);

			/**
			 * @brief Wrapper around CfgNode::opt
			 */
			CfgNode opt(CfgNode &&c);

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
			 * @brief Gets node or creates an empty one
			 */
			CfgNode &get(std::string const &name);

			/**
			 * @brief Gets node by name
			 */
			CfgNode const &get(std::string const &name) const;

			/**
			 * @brief Gets node by id
			 */
			CfgNode const &get(uint32_t id) const;

			CfgNode &operator[](std::string const &name) { return get(name); }
			CfgNode const &operator[](std::string const &name) const { return get(name); }
			CfgNode const &operator[](uint32_t id) const { return get(id); }

			void debug_node(CfgNode const &node, std::ostream &os) const;
			void debug_node(std::string const &name, std::ostream &os) const;
			std::string node_str(CfgNode const &name) const;
			std::string node_str(std::string const &name) const;

		private:
			std::map<std::string, uint32_t> _cfg_map;
			util::UIDList<CfgNode> _cfgs;
	};

	inline CfgNode operator+(CfgNode &&lhs, CfgNode &&rhs) {
		return CfgNode::seq(std::move(lhs), std::move(rhs));
	}
	inline CfgNode operator|(CfgNode &&lhs, CfgNode &&rhs) {
		return CfgNode::alt(std::move(lhs), std::move(rhs));
	}
}
