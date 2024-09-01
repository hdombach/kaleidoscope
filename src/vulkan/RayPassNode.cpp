#include "RayPassNode.hpp"
#include "RayPassMesh.hpp"
#include "RayPass.hpp"
#include "../util/format.hpp"

namespace vulkan {
	std::ostream& RayPassNode::VImpl::print_debug(std::ostream& os) const {
		return os << "{"
			<< "\"mesh_id\":" << mesh_id << ","
			<< "\"material_id\":" << material_id << ","
			<< "\"position\":" << position
			<< "}";
	}
	RayPassNode RayPassNode::create(const Node *node, const RayPass *ray_pass) {
		auto result = RayPassNode();

		result._node = node;
		result._ray_pass = ray_pass;
		result._vimpl = {
			ray_pass->mesh(node->mesh().id()).bvnode_id(),
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
