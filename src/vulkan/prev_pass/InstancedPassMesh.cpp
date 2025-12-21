#include "InstancedPassMesh.hpp"

#include <unordered_map>

#include "util/UIDList.hpp"
#include "util/log.hpp"
#include "vulkan/StaticBuffer.hpp"


namespace vulkan {
	using NodeVImpl = InstancedPassMesh::NodeVImpl;

	NodeVImpl::NodeVImpl():
		mesh_id(0),
		material_id(0),
		position(),
		transformation(),
		inverse_transformation()
	{ }

	NodeVImpl::NodeVImpl(vulkan::Node const &node):
		mesh_id(node.mesh().id()),
		material_id(node.material().id()),
		position(node.position()),
		transformation(node.get_matrix()),
		inverse_transformation(node.get_matrix_inverse())
	{ }

	bool NodeVImpl::has_value() const {
		log_assert((mesh_id == 0) == (material_id == 0), "If one id is 0, the other must be 0");
		return mesh_id != 0;
	}

	NodeVImpl::operator bool() const {
		return has_value();
	}

	util::Result<InstancedPassMesh, Error> InstancedPassMesh::create(const types::Mesh *mesh) {
		auto l = util::UIDList<NodeVImpl>();
		l.insert(NodeVImpl(), 0);

		InstancedPassMesh m;

		m._mesh = mesh;

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

		if (auto err = StaticBuffer::create(vertices).move_or(m._vertex_buffer)) {
			return Error(ErrorType::MISC, "Could not create vertex buffer for InstancedPassMesh", err.value());
		}

		if (auto err = StaticBuffer::create(indices).move_or(m._index_buffer)) {
			return Error(ErrorType::MISC, "Could not create index buffer for InstancedPassMesh", err.value());
		}

		return {std::move(m)};
	}

	InstancedPassMesh::InstancedPassMesh(InstancedPassMesh &&other) {
		_mesh = other._mesh;
		other._mesh = nullptr;

		_vertex_buffer = std::move(other._vertex_buffer);
		_index_buffer = std::move(other._index_buffer);
	}

	InstancedPassMesh &InstancedPassMesh::operator=(InstancedPassMesh &&other) {
		_mesh = other._mesh;
		other._mesh = nullptr;

		_vertex_buffer = std::move(other._vertex_buffer);
		_index_buffer = std::move(other._index_buffer);

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

	void InstancedPassMesh::add_node(vulkan::Node const &node) {
		log_assert(_nodes.insert(NodeVImpl(node), node.id()), "Couldn't insert node");
		if (auto err = _create_node_buffer().move_or()) {
			log_error() << "Couldn't create internal node buffer for instanced pass" << std::endl;
		}
	}

	void InstancedPassMesh::remove_node(uint32_t id) {
		log_assert(_nodes.contains(id), "Trying to remove node that doesn't exist");
		if (auto err = _create_node_buffer().move_or()) {
			log_error() << "Couldn't create internal node buffer after remove a node for the instanced pass" << std::endl;
		}
	}

	void InstancedPassMesh::destroy() {
		_mesh = nullptr;
		_vertex_buffer.destroy();
		_index_buffer.destroy();
	}

	InstancedPassMesh::~InstancedPassMesh() { destroy(); }

	util::Result<void, Error> InstancedPassMesh::_create_node_buffer() {
		if (auto err = StaticBuffer::create(_nodes.raw()).move_or(_node_buffer)) {
			return Error(ErrorType::MISC, "Could not allocate node buffer for InstancedPassMesh", err.value());
		}

		return {};
	}
}
