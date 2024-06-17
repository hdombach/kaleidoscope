#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "PreviewRenderPassMesh.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<PreviewRenderPassMesh, KError> PreviewRenderPassMesh::create(
			Scene &scene,
			const Mesh *mesh)
	{
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

		PreviewRenderPassMesh result;

		result._id = mesh->id();
		result._vertex_buffer_range = VkDeviceSize(sizeof(Vertex) * vertices.size());

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		Graphics::DEFAULT->create_buffer(
				result._vertex_buffer_range,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				staging_buffer,
				staging_buffer_memory);

		void *data;
		vkMapMemory(
				Graphics::DEFAULT->device(),
				staging_buffer_memory,
				0,
				result._vertex_buffer_range,
				0,
				&data);
		memcpy(
				data,
				vertices.data(),
				static_cast<size_t>(result._vertex_buffer_range));
		vkUnmapMemory(
				Graphics::DEFAULT->device(),
				staging_buffer_memory);

		Graphics::DEFAULT->create_buffer(
				result._vertex_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				result._vertex_buffer, 
				result._vertex_buffer_memory);

		Graphics::DEFAULT->copy_buffer(
				staging_buffer, 
				result._vertex_buffer, 
				result._vertex_buffer_range);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staging_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staging_buffer_memory, nullptr);

		result._index_buffer_range = sizeof(uint32_t) * indices.size();

		Graphics::DEFAULT->create_buffer(
				result._index_buffer_range, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staging_buffer, 
				staging_buffer_memory);

		vkMapMemory(
				Graphics::DEFAULT->device(),
				staging_buffer_memory,
				0,
				result._index_buffer_range,
				0,
				&data);
		memcpy(data, indices.data(), static_cast<size_t>(result._index_buffer_range));
		vkUnmapMemory(Graphics::DEFAULT->device(), staging_buffer_memory);

		Graphics::DEFAULT->create_buffer(
				result._index_buffer_range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_INDEX_BUFFER_BIT
				| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				result._index_buffer, 
				result._index_buffer_memory);

		Graphics::DEFAULT->copy_buffer(staging_buffer, result._index_buffer, result._index_buffer_range);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staging_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staging_buffer_memory, nullptr);

		result._index_count = indices.size();

		return {std::move(result)};
	}

	PreviewRenderPassMesh::PreviewRenderPassMesh(PreviewRenderPassMesh &&other)
	{
		_mesh = other._mesh;
		_id = other._id;

		_vertex_buffer = other._vertex_buffer;
		other._vertex_buffer = nullptr;

		_vertex_buffer_memory = other._vertex_buffer_memory;
		other._vertex_buffer_memory = nullptr;

		_index_buffer = other._index_buffer;
		other._index_buffer = nullptr;

		_index_buffer_memory = other._index_buffer_memory;
		other._index_buffer_memory = nullptr;

		_index_count = other._index_count;

		_vertex_buffer_range = other._vertex_buffer_range;
		_index_buffer_range = other._index_buffer_range;
	}

	PreviewRenderPassMesh& PreviewRenderPassMesh::operator=(PreviewRenderPassMesh&& other)
	{
		destroy();

		_mesh = other._mesh;
		_id = other._id;

		_vertex_buffer = other._vertex_buffer;
		other._vertex_buffer = nullptr;

		_vertex_buffer_memory = other._vertex_buffer_memory;
		other._vertex_buffer_memory = nullptr;

		_index_buffer = other._index_buffer;
		other._index_buffer = nullptr;

		_index_buffer_memory = other._index_buffer_memory;
		other._index_buffer_memory = nullptr;

		_index_count = other._index_count;

		_vertex_buffer_range = other._vertex_buffer_range;
		_index_buffer_range = other._index_buffer_range;

		return *this;
	}

	uint32_t PreviewRenderPassMesh::id() const {
		return _id;
	}

	VkBuffer PreviewRenderPassMesh::vertex_buffer() const {
		return _vertex_buffer;
	}

	VkBuffer PreviewRenderPassMesh::index_buffer() const {
		return _index_buffer;
	}

	size_t PreviewRenderPassMesh::index_count() const {
		return _index_count;
	}

	VkDeviceSize PreviewRenderPassMesh::vertex_buffer_range() const {
		return _vertex_buffer_range;
	}

	VkDeviceSize PreviewRenderPassMesh::index_buffer_range() const {
		return _index_buffer_range;
	}

	void PreviewRenderPassMesh::destroy() {
		if (_vertex_buffer) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), _vertex_buffer, nullptr);
			_vertex_buffer = nullptr;
		}
		if (_vertex_buffer_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _vertex_buffer_memory, nullptr);
			_vertex_buffer_memory = nullptr;
		}
		if (_index_buffer) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), _index_buffer, nullptr);
			_index_buffer = nullptr;
		}
		if (_index_buffer_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _index_buffer_memory, nullptr);
			_index_buffer_memory = nullptr;
		}
	}

	PreviewRenderPassMesh::~PreviewRenderPassMesh() {
		destroy();
	}

	PreviewRenderPassMesh::PreviewRenderPassMesh():
		_mesh(nullptr),
		_id(0),
		_vertex_buffer(nullptr),
		_vertex_buffer_memory(nullptr),
		_index_buffer(nullptr),
		_index_buffer_memory(nullptr)
	{
	}
}
