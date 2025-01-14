#include <vector>
#include <unordered_map>
#include <filesystem>
#include <string>

#include <tiny_obj_loader.h>

#include "StaticMesh.hpp"
#include "util/result.hpp"
#include "util/KError.hpp"
#include "vulkan/Vertex.hpp"

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

				vertices.push_back(vertex);
			}
		}
		auto name = std::filesystem::path(url).stem();
		return from_vertices(name, id, vertices);
	}

	StaticMesh::Ptr StaticMesh::create_square(std::string const &name, uint32_t id) {
		auto vertices = std::vector<vulkan::Vertex>{
			{{0.0, 0.2, 1.0}, {0.0, 0.0, 0.0}, {0.0, 1.0}},
			{{0.0, 0.1, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0}},
			{{1.0, 0.3, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0}},

			{{0.0, 0.2, 1.0}, {0.0, 0.0, 0.0}, {0.0, 1.0}},
			{{1.0, 0.3, 0.0}, {0.0, 0.0, 0.0}, {1.0, 0.0}},
			{{1.0, 0.4, 1.0}, {0.0, 0.0, 0.0}, {1.0, 1.0}},
		};

		return from_vertices(name, id, vertices);
	}

	StaticMesh::Ptr StaticMesh::from_vertices(
			std::string const &name,
			uint32_t id,
			std::vector<vulkan::Vertex> const &vertices)
	{
		auto result = std::unique_ptr<StaticMesh>(new StaticMesh());

		result->_id = id;
		result->_vertices = vertices;
		result->_name = name;
		return result;
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

	void StaticMesh::set_name(const std::string &name) {
		_name = name;
	}
	std::string const &StaticMesh::name() const {
		return _name;
	}
}
