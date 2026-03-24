#include "InstancedPassNode.hpp"
#include "util/Util.hpp"
#include "InstancedPass.hpp"

namespace vulkan {
	using VImpl = InstancedPassNode::VImpl;

	VImpl VImpl::create_empty() {
		return {0, 0, 0, glm::vec3(), glm::mat4(), glm::mat4()};
	}

	InstancedPassNode InstancedPassNode::create( const vulkan::Node &node) {
		auto n = InstancedPassNode();
		n._node = &node;
		return n;
	}

	InstancedPassNode::InstancedPassNode(InstancedPassNode &&other) {
		_node = util::move_ptr(other._node);
		registered_mesh = other.registered_mesh;
	}

	InstancedPassNode &InstancedPassNode::operator=(InstancedPassNode &&other) {
		_node = util::move_ptr(other._node);
		registered_mesh = other.registered_mesh;

		return *this;
	}

	bool InstancedPassNode::has_value() const {
		return _node != nullptr;
	}

	InstancedPassNode::operator bool() const {
		return has_value();
	}

	uint32_t InstancedPassNode::id() const {
		return _node->id();
	}

	types::Mesh const &InstancedPassNode::mesh() const {
		return _node->mesh();
	}

	vulkan::Node const &InstancedPassNode::node() const {
		return *_node;
	}

	VImpl InstancedPassNode::vimpl() const {
		return VImpl{
			_node->mesh().id(),
			_node->material().id(),
			_node->mesh().is_de(),
			_node->position(),
			_node->get_matrix_inverse(), // Swap these?
			_node->get_matrix(),
		};
	}

	void InstancedPassNode::destroy() {
		_node = nullptr;
	}

	InstancedPassNode::~InstancedPassNode() {
		destroy();
	}
}
