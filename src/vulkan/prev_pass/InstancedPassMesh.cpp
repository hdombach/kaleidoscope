#include "InstancedPassMesh.hpp"

#include <unordered_map>

#include "util/UIDList.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "vulkan/StaticBuffer.hpp"
#include "InstancedPass.hpp"


namespace vulkan {
	using NodeVImpl = InstancedPassMesh::NodeVImpl;

	NodeVImpl::NodeVImpl():
		node_id(0),
		material_id(0),
		position(),
		transformation(),
		inverse_transformation()
	{ }

	NodeVImpl::NodeVImpl(vulkan::Node const &node):
		node_id(node.id()),
		material_id(node.material().id()),
		position(node.position()),
		transformation(node.get_matrix()),
		inverse_transformation(node.get_matrix_inverse())
	{ }

	util::Result<InstancedPassMesh, Error> InstancedPassMesh::create(
		const types::Mesh *mesh,
		InstancedPass const &instanced_pass
	) {
		InstancedPassMesh m;

		m._mesh = mesh;
		m._instanced_pass = &instanced_pass;

		auto vertices = std::vector<vulkan::Vertex>();
		auto indices = std::vector<uint32_t>();
		auto unique_vertices = std::unordered_map<vulkan::Vertex, uint32_t>();
		for (auto &vertex : *mesh) {
			if (unique_vertices.count(vertex) == 0) {
				unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(unique_vertices[vertex]);
		}
		m._index_count = indices.size();

		if (!vertices.empty()) {
			if (auto err = StaticBuffer::create_vertices(vertices).move_or(m._vertex_buffer)) {
				return Error(ErrorType::MISC, "Could not create vertex buffer for InstancedPassMesh", err.value());
			}
			if (auto err = StaticBuffer::create_indices(indices).move_or(m._index_buffer)) {
				return Error(ErrorType::MISC, "Could not create index buffer for InstancedPassMesh", err.value());
			}
		}

		return {std::move(m)};
	}

	InstancedPassMesh::InstancedPassMesh(InstancedPassMesh &&other) {
		_mesh = util::move_ptr(other._mesh);
		_instanced_pass = util::move_ptr(other._instanced_pass);
		_nodes = std::move(other._nodes);
		_vertex_buffer = std::move(other._vertex_buffer);
		_index_buffer = std::move(other._index_buffer);
		_node_buffer = std::move(other._node_buffer);
		_descriptor_set = std::move(other._descriptor_set);
		_index_count = other._index_count;
	}

	InstancedPassMesh &InstancedPassMesh::operator=(InstancedPassMesh &&other) {
		_mesh = util::move_ptr(other._mesh);
		_instanced_pass = util::move_ptr(other._instanced_pass);
		_nodes = std::move(other._nodes);
		_vertex_buffer = std::move(other._vertex_buffer);
		_index_buffer = std::move(other._index_buffer);
		_node_buffer = std::move(other._node_buffer);
		_descriptor_set = std::move(other._descriptor_set);
		_index_count = other._index_count;

		return *this;
	}

	bool InstancedPassMesh::has_value() const {
		return _mesh;
	}

	InstancedPassMesh::operator bool() const {
		return has_value();
	}

	bool InstancedPassMesh::is_de() const {
		return _mesh->is_de();
	}

	uint32_t InstancedPassMesh::id() const {
		return _mesh->id();
	}

	StaticBuffer const &InstancedPassMesh::vertex_buffer() const {
		return _vertex_buffer;
	}

	StaticBuffer const &InstancedPassMesh::index_buffer() const {
		return _index_buffer;
	}

	StaticBuffer const &InstancedPassMesh::node_buffer() const {
		return _node_buffer;
	}

	uint32_t InstancedPassMesh::index_count() const {
		return _index_count;
	}

	uint32_t InstancedPassMesh::instance_count() const {
		return _nodes.size();
	}

	void InstancedPassMesh::add_node(vulkan::Node const &node) {
		log_trace() << "Adding instace of mesh " << node.mesh().id() << std::endl;
		_nodes.push_back(NodeVImpl(node));
		if (auto err = _create_node_buffer().move_or()) {
			log_error() << "Couldn't create internal node buffer for instanced pass" << std::endl;
		}
	}

	DescriptorSets &InstancedPassMesh::descriptor_set() {
		return _descriptor_set;
	}

	void InstancedPassMesh::remove_node(uint32_t id) {
		auto count = std::erase_if(_nodes, [id](NodeVImpl &n) {return n.node_id == id;});
		log_assert(count == 1, util::f("Only 1 node with id ", id, " should exist"));
		if (auto err = _create_node_buffer().move_or()) {
			log_error() << "Couldn't create internal node buffer after remove a node for the instanced pass.\n" << err.value() << std::endl;
		}
	}

	util::Result<void, Error> InstancedPassMesh::update_node(vulkan::Node const &node) {
		NodeVImpl *vnode = nullptr;
		for (auto &n : _nodes) {
			if (n.node_id == node.id()) {
				vnode = &n;
			}
		}
		log_assert(vnode, util::f("NodeVImpl ", node.id(), " does not exist"));

		*vnode = NodeVImpl(node);

		if (auto err = _create_node_buffer().move_or()) {
			return Error(ErrorType::MISC, "Could recreate node buffer", err.value());
		}

		return {};
	}

	void InstancedPassMesh::destroy() {
		_mesh = nullptr;
		_instanced_pass = nullptr;

		_nodes.clear();

		_vertex_buffer.destroy();
		_index_buffer.destroy();
		_node_buffer.destroy();
		_descriptor_set.destroy();
	}

	InstancedPassMesh::~InstancedPassMesh() { destroy(); }

	util::Result<void, Error> InstancedPassMesh::_create_node_buffer() {
		if (_nodes.size() == 0) {
			_node_buffer.destroy();
			return {};
		}

		if (auto err = StaticBuffer::create(_nodes).move_or(_node_buffer)) {
			return Error(ErrorType::MISC, "Could not allocate node buffer for InstancedPassMesh.", err.value());
		}

		auto attachments = _instanced_pass->mesh_descriptor_set_layout().desc_attachments();
		log_assert(attachments.size() >= 1, "There are not enough desc attachments");
		attachments[0].add_buffer(_node_buffer);

		if (auto err = DescriptorSets::create(
				attachments,
				_instanced_pass->mesh_descriptor_set_layout(),
				_instanced_pass->descriptor_pool()
		).move_or(_descriptor_set)) {
			return Error(ErrorType::VULKAN, "Could not create instaced pass mesh descriptor set", err.value());
		}

		return {};
	}
}
