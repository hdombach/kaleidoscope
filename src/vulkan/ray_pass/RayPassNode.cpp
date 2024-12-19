#include "RayPassNode.hpp"
#include "RayPassMesh.hpp"
#include "RayPass.hpp"
#include "util/format.hpp"

namespace vulkan {
	RayPassNode::VImpl RayPassNode::VImpl::create_empty() {
		return RayPassNode::VImpl {
			0,
			0, // References the empty mesh
			0, // Not sure if this matters
			glm::mat4(1.0)
		};
	}

	std::ostream& RayPassNode::VImpl::print_debug(std::ostream& os) const {
		return os << "{"
			<< "\"node_id\":" << node_id << ","
			<< "\"mesh_id\":" << mesh_id << ","
			<< "\"material_id\":" << material_id << ","
			<< "\"object_transformation\":" << object_transformation
			<< "}";
	}
	util::Result<RayPassNode, KError> RayPassNode::create(const Node *node, const RayPass *ray_pass) {
		auto result = RayPassNode();

		if (node == nullptr) {
			return KError::invalid_arg("node is nullptr");
		}
		if (ray_pass == nullptr) {
			return KError::invalid_arg("ray_pass is nullptr");
		}

		result._node = node;
		result._ray_pass = ray_pass;

		return result;
	}

	RayPassNode::VImpl RayPassNode::vimpl() const {
		if (_node) {
			return VImpl{
				_node->id(),
					_ray_pass->mesh(_node->mesh().id()).bvnode_id(),
					_node->material().id(),
					_node->get_matrix_inverse(),
			};
		} else {
			return VImpl::create_empty();
		}
	}

	RayPassNode::RayPassNode():
		_node(nullptr),
		_ray_pass(nullptr)
	{ }
}
