#pragma once

#include <cstdint>
#include <ostream>

#include "../types/Node.hpp"

namespace vulkan {
	class RayPass;

	class RayPassNode {
		public:
			struct VImpl {
				alignas(4) uint32_t node_id;
				alignas(4) uint32_t mesh_id;
				alignas(4) uint32_t material_id;
				alignas(16) glm::vec3 position;

				static VImpl create_empty();

				static constexpr const char *declaration() {
					return
					"struct Node {\n"
					"\tuint node_id;\n"
					"\tuint mesh_id;\n"
					"\tuint material_id;\n"
					"\tvec3 position;\n"
					"};\n";
				}

				std::ostream& print_debug(std::ostream& os) const;
			} __attribute__((packed));

			RayPassNode();
			static RayPassNode create(const Node *node, const RayPass *ray_pass);

			VImpl vimpl() const;

			operator bool() const { return _node; }

			Node const &get() const { return *_node; }

		private:
			const Node *_node;
			const RayPass *_ray_pass;
	};
}

inline std::ostream& operator<<(std::ostream& os, vulkan::RayPassNode::VImpl const &node) {
	return node.print_debug(os);
}
