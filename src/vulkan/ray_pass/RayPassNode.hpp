#pragma once

#include <cstdint>
#include <ostream>

#include "types/Node.hpp"

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

				/**
				 * @brief Vulkan source code declaration for node
				 */
				static constexpr const char *declaration() {
					return
					"struct Node {\n"
					"\tuint node_id;\n"
					"\tuint mesh_id;\n"
					"\tuint material_id;\n"
					"\tmat4 object_transformation;\n"
					"};\n";
				}

				std::ostream& print_debug(std::ostream& os) const;
			} __attribute__((packed));

			RayPassNode();
			static util::Result<RayPassNode, KError> create(const Node *node, const RayPass *ray_pass);

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
