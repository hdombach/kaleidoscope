#pragma once

#include "../util/errors.hpp"
#include "../vulkan/mesh.hpp"
#include "../util/result.hpp"
#include <vulkan/vulkan.h>
#include "vertex.hpp"
#include "vulkan/vulkan_core.h"


namespace vulkan {
	class StaticMesh: public Mesh {
		public:
			static util::Result<StaticMesh *, KError> fromFile(std::string const &url);
			static StaticMesh *fromVertices(std::vector<Vertex> const &vertices, std::vector<uint32_t> const &indices);

			~StaticMesh();

			VkBuffer vertexBuffer() const;
			VkBuffer indexBuffer() const;
			uint32_t indexCount() const;

		private:
			StaticMesh() = default;

			VkBuffer vertexBuffer_;
			VkDeviceMemory vertexBufferMemory_;
			VkBuffer indexBuffer_;
			VkDeviceMemory indexBufferMemory_;
			uint32_t indexCount_;

	};
}