#include "staticMesh.hpp"
#include <tiny_obj_loader.h>
#include <unordered_map>
#include "graphics.hpp"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	util::Result<StaticMesh *, errors::InvalidMeshFile> StaticMesh::fromFile(
			const std::string &url)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		std::vector<vulkan::Vertex> vertices;
		std::vector<uint32_t> indices;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, url.c_str())) {
			return errors::InvalidMeshFile{util::f("Cannot load mesh ", url, ": ", warn, err)};
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

		return fromVertices(vertices, indices);
	}

	StaticMesh *StaticMesh::fromVertices(
			const std::vector<Vertex> &vertices,
			const std::vector<uint32_t> &indices)
	{
		auto result = new StaticMesh();

		auto bufferSize = VkDeviceSize(sizeof(vertices[0]) * vertices.size());

		VkBuffer staginBuffer;
		VkDeviceMemory staginBufferMemory;
		Graphics::DEFAULT->createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staginBuffer, 
				staginBufferMemory);

		void *data;
		vkMapMemory(
				Graphics::DEFAULT->device(),
				staginBufferMemory,
				0,
				bufferSize,
				0,
				&data);
		memcpy(data, vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(Graphics::DEFAULT->device(), staginBufferMemory);

		Graphics::DEFAULT->createBuffer(
				bufferSize, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				result->vertexBuffer_, 
				result->vertexBufferMemory_);

		Graphics::DEFAULT->copyBuffer(staginBuffer, result->vertexBuffer_, bufferSize);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staginBuffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staginBufferMemory, nullptr);

		bufferSize = sizeof(indices[0]) * indices.size();

		Graphics::DEFAULT->createBuffer(
				bufferSize, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staginBuffer, 
				staginBufferMemory);

		vkMapMemory(
				Graphics::DEFAULT->device(),
				staginBufferMemory,
				0,
				bufferSize,
				0,
				&data);
		memcpy(data, indices.data(), (size_t) bufferSize);
		vkUnmapMemory(Graphics::DEFAULT->device(), staginBufferMemory);

		Graphics::DEFAULT->createBuffer(
				bufferSize, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				result->indexBuffer_, 
				result->indexBufferMemory_);

		Graphics::DEFAULT->copyBuffer(staginBuffer, result->indexBuffer_, bufferSize);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staginBuffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staginBufferMemory, nullptr);

		result->indexCount_ = indices.size();

		return result;
	}

	StaticMesh::~StaticMesh() {
		vkDestroyBuffer(Graphics::DEFAULT->device(), vertexBuffer_, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), vertexBufferMemory_, nullptr);
		vkDestroyBuffer(Graphics::DEFAULT->device(), indexBuffer_, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), indexBufferMemory_, nullptr);
	}

	VkBuffer StaticMesh::vertexBuffer() const {
		return vertexBuffer_;
	}
	VkBuffer StaticMesh::indexBuffer() const {
		return indexBuffer_;
	}
	uint32_t StaticMesh::indexCount() const {
		return indexCount_;
	}
}
