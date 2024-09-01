#pragma once

#include <cstdint>
#include <ostream>

#include "../types/Node.hpp"

namespace vulkan {
	class RayPass;

	class RayPassNode {
		public:
			struct VImpl {
				alignas(4) uint32_t mesh_id;
				alignas(4) uint32_t material_id;
				alignas(16) glm::vec3 position;

				static constexpr const char *declaration() {
					return
					"struct Node {\n"
					"	uint mesh_id;\n"
					"	uint material_id;\n"
					"	vec3 position;\n"
					"};\n";
				}

				std::ostream& print_debug(std::ostream& os) const;
			} __attribute__((packed));

			RayPassNode();
			static RayPassNode create(const Node *node, const RayPass *ray_pass);

			VImpl vimpl() const { return _vimpl; }

			Node const &get() const { return *_node; }

		private:
			const Node *_node;
			const RayPass *_ray_pass;

			struct VImpl _vimpl;
	};
}

inline std::ostream& operator<<(std::ostream& os, vulkan::RayPassNode::VImpl const &node) {
	return node.print_debug(os);
}
