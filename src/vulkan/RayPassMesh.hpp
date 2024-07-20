#pragma once

#include <cstdint>

#include "../types/Mesh.hpp"

namespace vulkan {
	class RayPass;
	/* Reference to a single buffer that contains every vertice for all the meshes */
	class RayPassMesh {
		public:
			struct VImpl {
				uint32_t vertex_offset;
				uint32_t vertex_size;

				uint32_t index_offset;
				uint32_t index_size;

				static constexpr const char *declaration() {
					return
					"struct Mesh {\n"
					"	uint vertex_offset;\n"
					"	uint vertex_size;\n"
					"	uint index_offset;\n"
					"	uint index_size;\n"
					"};\n";
				}
			} __attribute__((packed));

			RayPassMesh(
					const types::Mesh *mesh,
					RayPass *ray_pass):
				_mesh(mesh),
				_ray_pass(ray_pass)
		{ }

			void update_offsets(
					uint32_t vertex_offset,
					uint32_t vertex_size,
					uint32_t index_offset,
					uint32_t index_size)
			{
				_vimpl = {
					vertex_offset,
					vertex_size,
					index_offset,
					index_size,
				};
			}

			uint32_t vertex_offset() const { return _vimpl.vertex_offset; }
			uint32_t vertex_size() const { return _vimpl.vertex_size; }

			uint32_t index_offset() const { return _vimpl.index_offset; }
			uint32_t index_size() const { return _vimpl.index_size; }

			VImpl vimpl() const { return _vimpl; }

			const types::Mesh *base_mesh() { return _mesh; }

		private:
			const types::Mesh *_mesh;
			const RayPass *_ray_pass;

			struct VImpl _vimpl;
	};
}
