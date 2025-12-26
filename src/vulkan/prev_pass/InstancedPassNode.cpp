#include "InstancedPassNode.hpp"
#include "util/Util.hpp"
#include "InstancedPass.hpp"

namespace vulkan {
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

	void InstancedPassNode::destroy() {
		_node = nullptr;
	}

	InstancedPassNode::~InstancedPassNode() {
		destroy();
	}
}
