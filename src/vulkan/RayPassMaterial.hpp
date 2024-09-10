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

			std::string const &resource_declaration() const { return _resource_declaration; }
			std::string const &material_buf() const { return _material_buf; }
			std::string const &frag_src() const { return _frag_src; }
			std::string const &material_frag_call() {
				if (_frag_call.empty()) {
					_create_frag_call();
				}
				return _frag_call;
			}
			const types::Material *get() const { return _material; }
			void update();

		private:
			const types::Material *_material;
			const RayPass *_ray_pass;

			std::string _resource_declaration;
			std::string _material_buf;
			std::string _frag_src;
			std::string _frag_call;

		private:
			void _create_declaration();
			void _create_buf_stuff();
			void _create_frag_src();
			void _create_frag_call();
	};
}
