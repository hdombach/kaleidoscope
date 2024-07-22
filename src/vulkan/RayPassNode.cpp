#include "RayPassNode.hpp"

namespace vulkan {
	RayPassNode RayPassNode::create(const Node *node, const RayPass *ray_pass) {
		auto result = RayPassNode();

		result._node = node;
		result._ray_pass = ray_pass;
		result._vimpl = {
			node->mesh().id(),
			node->material().id(),
			node->position()
		};

		return result;
	}

	RayPassNode::RayPassNode():
		_node(nullptr),
		_ray_pass(nullptr),
		_vimpl{}
	{ }
}
