#include <cstdint>

namespace vulkan {
	class RayPassMesh {
		public:
			RayPassMesh();

			uint32_t vertex_offset() const { return _vertex_offset; }
			uint32_t vertex_size() const { return _vertex_size; }

			uint32_t index_offset() const { return _index_offset; }
			uint32_t index_size() const { return _index_size; }

		private:
			uint32_t _vertex_offset;
			uint32_t _vertex_size;

			uint32_t _index_offset;
			uint32_t _index_size;
	};
}
