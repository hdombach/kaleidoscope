#include <tiny_obj_loader.h>
#include <vector>
#include <unordered_map>

#include "StaticMesh.hpp"
#include "../vulkan/graphics.hpp"
#include "vulkan/vulkan_core.h"

namespace types {
	util::Result<StaticMesh *, KError> StaticMesh::from_file(
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

	StaticMesh *StaticMesh::create_square(uint32_t id) {
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

	StaticMesh *StaticMesh::from_vertices(
			uint32_t id,
			const std::vector<vulkan::Vertex> &vertices,
			const std::vector<uint32_t> &indices)
	{
		auto result = new StaticMesh();

		result->_id = id;
		result->_vertices = vertices;
		result->_vertex_buffer_range = VkDeviceSize(sizeof(result->_vertices[0]) * result->_vertices.size());

		VkBuffer staginBuffer;
		VkDeviceMemory staginBufferMemory;
		vulkan::Graphics::DEFAULT->create_buffer(
				result->_vertex_buffer_range,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staginBuffer, 
				staginBufferMemory);

		void *data;
		vkMapMemory(
				vulkan::Graphics::DEFAULT->device(),
				staginBufferMemory,
				0,
				result->_vertex_buffer_range,
				0,
				&data);
		memcpy(data, result->_vertices.data(), (size_t) result->_vertex_buffer_range);
		vkUnmapMemory(vulkan::Graphics::DEFAULT->device(), staginBufferMemory);

		vulkan::Graphics::DEFAULT->create_buffer(
				result->_vertex_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT
					| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
					| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				result->_vertex_buffer, 
				result->_vertex_buffer_memory);

		vulkan::Graphics::DEFAULT->copy_buffer(
				staginBuffer,
				result->_vertex_buffer,
				result->_vertex_buffer_range);

		vkDestroyBuffer(vulkan::Graphics::DEFAULT->device(), staginBuffer, nullptr);
		vkFreeMemory(vulkan::Graphics::DEFAULT->device(), staginBufferMemory, nullptr);

		result->_index_buffer_range = sizeof(indices[0]) * indices.size();

		vulkan::Graphics::DEFAULT->create_buffer(
				result->_index_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staginBuffer, 
				staginBufferMemory);

		vkMapMemory(
				vulkan::Graphics::DEFAULT->device(),
				staginBufferMemory,
				0,
				result->_index_buffer_range,
				0,
				&data);
		memcpy(data, indices.data(), (size_t) result->_index_buffer_range);
		vkUnmapMemory(vulkan::Graphics::DEFAULT->device(), staginBufferMemory);

		vulkan::Graphics::DEFAULT->create_buffer(
				result->_index_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT
					| VK_BUFFER_USAGE_INDEX_BUFFER_BIT
					| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				result->_index_buffer, 
				result->_index_buffer_memory);

		vulkan::Graphics::DEFAULT->copy_buffer(staginBuffer, result->_index_buffer, result->_index_buffer_range);

		vkDestroyBuffer(vulkan::Graphics::DEFAULT->device(), staginBuffer, nullptr);
		vkFreeMemory(vulkan::Graphics::DEFAULT->device(), staginBufferMemory, nullptr);

		result->_index_count = indices.size();

		return result;
	}

	StaticMesh *StaticMesh::from_vertices(
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

	StaticMesh::StaticMesh(StaticMesh &&other) {
		_vertex_buffer = other._vertex_buffer;
		other._vertex_buffer = nullptr;

		_vertex_buffer_memory = other._vertex_buffer_memory;
		other._vertex_buffer_memory = nullptr;

		_index_buffer = other._index_buffer;
		other._index_buffer = nullptr;

		_index_buffer_memory = other._index_buffer_memory;
		other._index_buffer_memory = nullptr;
	}

	StaticMesh& StaticMesh::operator=(StaticMesh&& other) {
		destroy();

		_vertex_buffer = other._vertex_buffer;
		other._vertex_buffer = nullptr;

		_vertex_buffer_memory = other._vertex_buffer_memory;
		other._vertex_buffer_memory = nullptr;

		_index_buffer = other._index_buffer;
		other._index_buffer = nullptr;

		_index_buffer_memory = other._index_buffer_memory;
		other._index_buffer_memory = nullptr;

		return *this;
	}

	void StaticMesh::destroy() {
		if (_vertex_buffer) {
			vkDestroyBuffer(vulkan::Graphics::DEFAULT->device(), _vertex_buffer, nullptr);
			_vertex_buffer = nullptr;
		}
		if (_vertex_buffer_memory) {
			vkFreeMemory(vulkan::Graphics::DEFAULT->device(), _vertex_buffer_memory, nullptr);
			_vertex_buffer_memory = nullptr;
		}
		if (_index_buffer) {
			vkDestroyBuffer(vulkan::Graphics::DEFAULT->device(), _index_buffer, nullptr);
			_index_buffer = nullptr;
		}
		if (_index_buffer_memory) {
			vkFreeMemory(vulkan::Graphics::DEFAULT->device(), _index_buffer_memory, nullptr);
			_index_buffer_memory = nullptr;
		}
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

	VkBuffer StaticMesh::vertex_buffer() const {
		return _vertex_buffer;
	}
	VkBuffer StaticMesh::index_buffer() const {
		return _index_buffer;
	}
	uint32_t StaticMesh::index_count() const {
		return _index_count;
	}
	VkDeviceSize StaticMesh::vertex_buffer_range() const {
		return _vertex_buffer_range;
	}
	VkDeviceSize StaticMesh::index_buffer_range() const {
		return _index_buffer_range;
	}
}
