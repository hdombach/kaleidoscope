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

			std::string &resource_declaration() { return _resource_declaration; }

		private:
			const types::Material *_material;
			const RayPass *_ray_pass;

			std::string _resource_declaration;
	};
}
