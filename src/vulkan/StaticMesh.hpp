#pragma once

#include "../util/errors.hpp"
#include "../vulkan/Mesh.hpp"
#include "../util/result.hpp"
#include <vulkan/vulkan.h>
#include "Vertex.hpp"
#include "vulkan/vulkan_core.h"


namespace vulkan {
	class StaticMesh: public Mesh {
		public:
			static util::Result<StaticMesh *, KError> from_file(std::string const &url);
			static StaticMesh *from_vertices(std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices);

			~StaticMesh();

			VkBuffer vertex_buffer() const;
			VkBuffer index_buffer() const;
			uint32_t index_count() const;

		private:
			StaticMesh() = default;

			VkBuffer _vertex_buffer;
			VkDeviceMemory _vertex_buffer_memory;
			VkBuffer _index_buffer;
			VkDeviceMemory _index_buffer_memory;
			uint32_t _index_count;

	};
}