#pragma once

#include <cstdint>
#include <ostream>

#include "types/Node.hpp"
#include "vulkan/TemplUtils.hpp"

namespace vulkan {
	class RayPass;

	class RayPassNode {
		public:
			/**
			 * @brief Vulkan representation of a node
			 */
			struct VImpl {
				/**
				 * @brief id of original node
				 * Used for adding outline to nodes
				 */
				alignas(4) uint32_t node_id;
				/**
				 * @brief Index to vulkan bvnode
				 */
				alignas(4) uint32_t mesh_id;
				/**
				 * @brief Index to vulkan material
				 */
				alignas(4) uint32_t material_id;
				/**
				 * @brief Object transformation of node
				 */
				alignas(16) glm::mat4 object_transformation;

				/**
				 * @brief Creates node with id of 0
				 */
				static VImpl create_empty();

				inline const static auto declaration = std::vector{
					templ_property("uint", "node_id"),
					templ_property("uint", "mesh_id"),
					templ_property("uint", "material_id"),
					templ_property("mat4", "object_transformation")
				};

				std::ostream& print_debug(std::ostream& os) const;
			} __attribute__((packed));

			RayPassNode();
			static util::Result<RayPassNode, KError> create(const Node *node, const RayPass *ray_pass);

			void destroy();

			/**
			 * @brief Creates vulkan implimentation
			 */
			VImpl vimpl() const;

			bool has_value() const { return _node; }
			operator bool() const { return has_value(); }

			uint32_t id() const { return _node->id(); }

			/**
			 * @brief Gets underlying generic node
			 */
			Node const &get() const { return *_node; }

		private:
			const Node *_node;
			const RayPass *_ray_pass;
	};
}

inline std::ostream& operator<<(std::ostream& os, vulkan::RayPassNode::VImpl const &node) {
	return node.print_debug(os);
}
