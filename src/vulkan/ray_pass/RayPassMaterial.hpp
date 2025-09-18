#pragma once

#include <string>
#include "util/result.hpp"
#include "util/KError.hpp"
#include "types/Material.hpp"
#include "codegen/TemplObj.hpp"

namespace vulkan {
	class RayPass;
	class Node;

	class RayPassMaterial {
		public:
			RayPassMaterial();
			static util::Result<RayPassMaterial, KError> create(
					const types::Material *material,
					const RayPass *ray_pass);

			bool has_value() const { return _material; }
			operator bool() const { return has_value(); }
			uint32_t id() const { return _material->id(); }

			/**
			 * @brief Gets underlying generic material
			 */
			const types::Material *get() const { return _material; }
			void update();

		private:
			const types::Material *_material;
			const RayPass *_ray_pass;

			std::string _cg_struct_decl;
			std::string _cg_buf_decl;
			std::string _cg_frag_def;
			std::string _cg_frag_call;
	};
}
