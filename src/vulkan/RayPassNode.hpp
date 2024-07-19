#pragma once

#include <cstdint>

#include "../types/Node.hpp"

namespace vulkan {
	class RayPass;

	class RayPassNode {
		public:
			struct VImpl {
				uint32_t mesh_id;

				static constexpr const char *declaration() {
					return
					"struct Node {\n"
					"	uint mesh_id;"
					"}\n";
				}
			} __attribute__((packed));

			RayPassNode(const Node *node, RayPass *ray_pass) {
				_node = node;
				_vimpl = {
					_node->mesh().id()
				};
			}

			VImpl vimpl() const { return _vimpl; }

		private:
			const Node *_node;
			const RayPass *_ray_pass;

			struct VImpl _vimpl;
	};
}
