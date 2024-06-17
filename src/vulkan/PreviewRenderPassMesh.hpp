#pragma once

#include <vulkan/vulkan.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "Mesh.hpp"

namespace vulkan {
	class Scene;

	class PreviewRenderPassMesh {
		public:
			PreviewRenderPassMesh();

			static util::Result<PreviewRenderPassMesh, KError> create(Scene &scene, const Mesh *mesh);

			PreviewRenderPassMesh(const PreviewRenderPassMesh& other) = delete;
			PreviewRenderPassMesh(PreviewRenderPassMesh &&other);
			PreviewRenderPassMesh& operator=(const PreviewRenderPassMesh& other) = delete;
			PreviewRenderPassMesh& operator=(PreviewRenderPassMesh&& other);

			bool exists() const;
			operator bool() { return exists(); }

			uint32_t id() const;
			VkBuffer vertex_buffer() const;
			VkBuffer index_buffer() const;
			size_t index_count() const;
			VkDeviceSize vertex_buffer_range() const;
			VkDeviceSize index_buffer_range() const;

			void destroy();
			~PreviewRenderPassMesh();
		private:
			const Mesh *_mesh;
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
