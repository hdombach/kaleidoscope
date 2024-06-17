#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "../vulkan/Mesh.hpp"
#include "Vertex.hpp"


namespace vulkan {
	class StaticMesh: public Mesh {
		public:
			static util::Result<StaticMesh *, KError> from_file(uint32_t id, std::string const &url);
			static StaticMesh *create_square(uint32_t id);
			static StaticMesh *from_vertices(uint32_t id, std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices);
			static StaticMesh *from_vertices(uint32_t id, std::vector<Vertex> const &vertices);

			StaticMesh(const StaticMesh& other) = delete;
			StaticMesh(StaticMesh &&other);
			StaticMesh& operator=(const StaticMesh& other) = delete;
			StaticMesh& operator=(StaticMesh&& other);

			void destroy();
			~StaticMesh();

			const_iterator begin() const;
			const_iterator end() const;
			uint32_t id() const;
			size_t size() const;

			VkBuffer vertex_buffer() const;
			VkBuffer index_buffer() const;
			uint32_t index_count() const;
			VkDeviceSize vertex_buffer_range() const;
			VkDeviceSize index_buffer_range() const;

		private:
			StaticMesh() = default;

			std::vector<Vertex> _vertices;
			uint32_t _id;

			VkBuffer _vertex_buffer;
			VkDeviceMemory _vertex_buffer_memory;
			VkBuffer _index_buffer;
			VkDeviceMemory _index_buffer_memory;
			uint32_t _index_count;
			VkDeviceSize _vertex_buffer_range;
			VkDeviceSize _index_buffer_range;
	};
}
