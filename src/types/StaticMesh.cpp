#include <tiny_obj_loader.h>
#include <vector>
#include <unordered_map>

#include "StaticMesh.hpp"

namespace types {
	util::Result<StaticMesh::Ptr , KError> StaticMesh::from_file(
			uint32_t id,
			const std::string &url)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		std::vector<vulkan::Vertex> vertices;
		std::vector<uint32_t> indices;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, url.c_str())) {
			return KError::invalid_mesh_file("Cannot load mesh " + url + ": " + warn + err);
		}

		std::unordered_map<vulkan::Vertex, uint32_t> uniqueVertices{};

		for (const auto &shape : shapes) {
			for (const auto &index : shape.mesh.indices) {
				vulkan::Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};
				vertex.tex_coord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0 - attrib.texcoords[2 * index.texcoord_index + 1],
				};

				vertex.color = {1.0f, 1.0f, 1.0f};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}

		return from_vertices(id, vertices, indices);
	}

	StaticMesh::Ptr StaticMesh::create_square(uint32_t id) {
		auto vertices = std::vector<vulkan::Vertex>{
			{{0.0, 0.2, 1.0}, {0.0, 0.0, 0.0}, {0.0, 1.0}},
			{{0.0, 0.1, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
			{{1.0, 0.3, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0}},

			{{0.0, 0.2, 1.0}, {0.0, 0.0, 0.0}, {0.0, 1.0}},
			{{1.0, 0.3, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0}},
			{{1.0, 0.4, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0}},
		};

		auto indices = std::vector<uint32_t>{
			1, 0, 2,
			1, 2, 3,
		};
		return from_vertices(id, vertices, indices);
	}

	StaticMesh::Ptr StaticMesh::from_vertices(
			uint32_t id,
			const std::vector<vulkan::Vertex> &vertices,
			const std::vector<uint32_t> &indices)
	{
		auto result = std::unique_ptr<StaticMesh>(new StaticMesh());

		result->_id = id;
		result->_vertices = vertices;
		return result;
	}

	StaticMesh::Ptr StaticMesh::from_vertices(
			uint32_t id,
			std::vector<vulkan::Vertex> const &vertices)
	{
		auto vertices_result = std::vector<vulkan::Vertex>();
		auto indices_result = std::vector<uint32_t>();
		auto unique_vertices = std::unordered_map<vulkan::Vertex, uint32_t>();
		for (auto &vertex : vertices) {
			if (unique_vertices.count(vertex) == 0) {
				unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices_result.push_back(vertex);
			}
			indices_result.push_back(unique_vertices[vertex]);
		}
		return from_vertices(id, vertices_result, indices_result);
	}

	void StaticMesh::destroy() {
		_vertices.clear();
	}

	StaticMesh::~StaticMesh() {
		destroy();
	}

	Mesh::const_iterator StaticMesh::begin() const {
		return _vertices.data();
	}

	Mesh::const_iterator StaticMesh::end() const {
		return _vertices.data() + _vertices.size();
	}

	uint32_t StaticMesh::id() const {
		return _id;
	}

	size_t StaticMesh::size() const {
		return _vertices.size();
	}
}
