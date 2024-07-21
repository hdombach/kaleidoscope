#pragma once

#include <cstdint>

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
			} __attribute__((packed));

			RayPassNode(const Node *node, RayPass *ray_pass) {
				_node = node;
				_vimpl = {
					_node->mesh().id(),
					_node->material().id(),
					_node->position(),
				};
			}

			VImpl vimpl() const { return _vimpl; }

		private:
			const Node *_node;
			const RayPass *_ray_pass;

			struct VImpl _vimpl;
	};
}
