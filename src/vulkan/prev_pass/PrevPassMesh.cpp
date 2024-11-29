#include <unordered_map>
#include <vulkan/vulkan_core.h>

#include "PrevPassMesh.hpp"
#include "vulkan/graphics.hpp"

namespace vulkan {
	util::Result<PrevPassMesh, KError> PrevPassMesh::create(
			Scene &scene,
			const types::Mesh *mesh)
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

		PrevPassMesh result;

		result._mesh = mesh;
		result._id = mesh->id();
		result._vertex_buffer_range = VkDeviceSize(sizeof(Vertex) * vertices.size());

		if (vertices.size() > 0) {
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
		}

		result._index_buffer_range = sizeof(uint32_t) * indices.size();

		if (indices.size() > 0) {
			VkBuffer staging_buffer;
			VkDeviceMemory staging_buffer_memory;
			Graphics::DEFAULT->create_buffer(
					result._index_buffer_range, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					staging_buffer, 
					staging_buffer_memory);

			void *data;
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
		}

		result._index_count = indices.size();

		return {std::move(result)};
	}

	PrevPassMesh::PrevPassMesh(PrevPassMesh &&other)
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

	PrevPassMesh& PrevPassMesh::operator=(PrevPassMesh&& other)
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

	const types::Mesh *PrevPassMesh::base() const {
		return _mesh;
	}

	bool PrevPassMesh::exists() const {
		return _mesh;
	}

	bool PrevPassMesh::is_de() {
		return _mesh->is_de();
	}

	uint32_t PrevPassMesh::id() const {
		return _id;
	}

	VkBuffer PrevPassMesh::vertex_buffer() const {
		return _vertex_buffer;
	}

	VkBuffer PrevPassMesh::index_buffer() const {
		return _index_buffer;
	}

	size_t PrevPassMesh::index_count() const {
		return _index_count;
	}

	VkDeviceSize PrevPassMesh::vertex_buffer_range() const {
		return _vertex_buffer_range;
	}

	VkDeviceSize PrevPassMesh::index_buffer_range() const {
		return _index_buffer_range;
	}

	void PrevPassMesh::destroy() {
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

	PrevPassMesh::~PrevPassMesh() {
		destroy();
	}

	PrevPassMesh::PrevPassMesh():
		_mesh(nullptr),
		_id(0),
		_vertex_buffer(nullptr),
		_vertex_buffer_memory(nullptr),
		_index_buffer(nullptr),
		_index_buffer_memory(nullptr)
	{
	}
}
