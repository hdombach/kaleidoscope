#include "StaticMesh.hpp"
#include <tiny_obj_loader.h>
#include <unordered_map>
#include "graphics.hpp"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	util::Result<StaticMesh *, KError> StaticMesh::from_file(
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
				vertex.texCoord = {
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

		return from_vertices(vertices, indices);
	}

	StaticMesh *StaticMesh::from_vertices(
			const std::vector<Vertex> &vertices,
			const std::vector<uint32_t> &indices)
	{
		auto result = new StaticMesh();

		result->_vertex_buffer_range = VkDeviceSize(sizeof(vertices[0]) * vertices.size());

		VkBuffer staginBuffer;
		VkDeviceMemory staginBufferMemory;
		Graphics::DEFAULT->create_buffer(
				result->_vertex_buffer_range,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staginBuffer, 
				staginBufferMemory);

		void *data;
		vkMapMemory(
				Graphics::DEFAULT->device(),
				staginBufferMemory,
				0,
				result->_vertex_buffer_range,
				0,
				&data);
		memcpy(data, vertices.data(), (size_t) result->_vertex_buffer_range);
		vkUnmapMemory(Graphics::DEFAULT->device(), staginBufferMemory);

		Graphics::DEFAULT->create_buffer(
				result->_vertex_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				result->_vertex_buffer, 
				result->_vertex_buffer_memory);

		Graphics::DEFAULT->copy_buffer(
				staginBuffer,
				result->_vertex_buffer,
				result->_vertex_buffer_range);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staginBuffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staginBufferMemory, nullptr);

		result->_index_buffer_range = sizeof(indices[0]) * indices.size();

		Graphics::DEFAULT->create_buffer(
				result->_index_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staginBuffer, 
				staginBufferMemory);

		vkMapMemory(
				Graphics::DEFAULT->device(),
				staginBufferMemory,
				0,
				result->_index_buffer_range,
				0,
				&data);
		memcpy(data, indices.data(), (size_t) result->_index_buffer_range);
		vkUnmapMemory(Graphics::DEFAULT->device(), staginBufferMemory);

		Graphics::DEFAULT->create_buffer(
				result->_index_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				result->_index_buffer, 
				result->_index_buffer_memory);

		Graphics::DEFAULT->copy_buffer(staginBuffer, result->_index_buffer, result->_index_buffer_range);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staginBuffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staginBufferMemory, nullptr);

		result->_index_count = indices.size();

		return result;
	}

	StaticMesh::~StaticMesh() {
		vkDestroyBuffer(Graphics::DEFAULT->device(), _vertex_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), _vertex_buffer_memory, nullptr);
		vkDestroyBuffer(Graphics::DEFAULT->device(), _index_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), _index_buffer_memory, nullptr);
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
