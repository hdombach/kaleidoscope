#pragma once

#include <vulkan/vulkan.h>

#include "util/result.hpp"
#include "util/BaseError.hpp"
#include "types/Mesh.hpp"

namespace vulkan {
	class Scene;

	class PrevPassMesh {
		public:
			enum class ErrorType {
				MISC
			};

			using Error = BaseError<ErrorType>;

			PrevPassMesh();

			static util::Result<PrevPassMesh, Error> create(Scene &scene, const types::Mesh *mesh);

			PrevPassMesh(const PrevPassMesh& other) = delete;
			PrevPassMesh(PrevPassMesh &&other);
			PrevPassMesh& operator=(const PrevPassMesh& other) = delete;
			PrevPassMesh& operator=(PrevPassMesh&& other);

			const types::Mesh *base() const;

			bool exists() const;
			operator bool() const { return exists(); }

			bool is_de();

			uint32_t id() const;
			VkBuffer vertex_buffer() const;
			VkBuffer index_buffer() const;
			size_t index_count() const;
			VkDeviceSize vertex_buffer_range() const;
			VkDeviceSize index_buffer_range() const;

			void destroy();
			~PrevPassMesh();
		private:
			const types::Mesh *_mesh;
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

template<>
	const char *vulkan::PrevPassMesh::Error::type_str(vulkan::PrevPassMesh::ErrorType t);

inline std::ostream &operator<<(std::ostream &os, vulkan::PrevPassMesh::ErrorType type) {
	return os << vulkan::PrevPassMesh::Error::type_str(type);
}
