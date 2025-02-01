#pragma once

#include "CfgContext.hpp"
#include "CfgNode.hpp"
#include "util/FileLocation.hpp"
#include "util/KError.hpp"

namespace cg {
	/**
	 * @brief Abstract syntax tree
	 * The type is specified by the cfg;
	 */

	class AstNode {
		public:
			AstNode();
			AstNode(
				uint32_t id,
				CfgContext const &ctx,
				uint32_t cfg_id,
				util::FileLocation const &file_location
			);

			uint32_t id() const { return _id; }

			bool has_value() const;
			operator bool() const { return has_value(); }

			std::vector<AstNode> const &children() const { return _children; }

			void add_child(AstNode const &node);

			util::Result<AstNode, KError> child_with_cfg(std::string const &name) const;

			std::vector<AstNode> children_with_cfg(std::string const &name) const;

			/**
			 * @brief The rule that was used to generate this node
			 */
			CfgNode const &cfg_node() const { return _ctx->get(_cfg_id); }

			std::string cfg_name() const { return cfg_node().name(); }

			std::string const &consumed() const { return _consumed; }

			void consume(char c);

			/**
			 * @brief Number of characters by this node and any children node
			 */
			size_t size() const;

			bool is_ref() const;

			/**
			 * @brief Combines all nodes that aren't in the list of provided cfg's
			 */
			void compress(std::vector<uint32_t> const &cfg_ids);
			util::Result<void, KError> compress(std::vector<std::string> const &cfg_names);

			AstNode compressed(std::vector<uint32_t> const &cfg_ids);
			util::Result<AstNode, KError> compressed(std::vector<std::string> const &cfg_names);

			void debug_pre_order(std::ostream &os) const;
			void debug_dot(std::ostream &os) const;

			std::string pre_order_str() const;

		private:
			uint32_t _id;
			uint32_t _cfg_id;
			CfgContext const *_ctx;
			std::vector<AstNode> _children;
			std::string _consumed;
			util::FileLocation _location;

			/**
			 * @brief The cached size
			 */
			mutable size_t _size;

		private:
			size_t _calc_size() const;
			void _debug_dot_attributes(std::ostream &os) const;
			void _debug_dot_paths(std::ostream &os) const;
	};
}
