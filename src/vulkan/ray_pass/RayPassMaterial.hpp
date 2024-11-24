#pragma once

#include <string>

namespace types {
	class Material;
}

namespace vulkan {
	class RayPass;
	class Node;

	class RayPassMaterial {
		public:
			RayPassMaterial();
			static RayPassMaterial create(
					const types::Material *material,
					const RayPass *ray_pass);

			std::string const &cg_struct_decl();
			std::string const &cg_buf_decl();
			std::string const &cg_frag_def();
			std::string const &cg_frag_call();
			const types::Material *get() const { return _material; }
			void update();

		private:
			const types::Material *_material;
			const RayPass *_ray_pass;

			std::string _cg_struct_decl;
			std::string _cg_buf_decl;
			std::string _cg_frag_def;
			std::string _cg_frag_call;

		private:
			void _create_struct_decl();
			void _create_buf_decl();
			void _create_frag_def();
			void _create_frag_call();
	};
}
