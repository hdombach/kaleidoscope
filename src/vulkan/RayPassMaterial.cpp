#include "RayPassMaterial.hpp"
#include "../types/Material.hpp"

namespace vulkan {
	RayPassMaterial::RayPassMaterial():
		_material(nullptr),
		_ray_pass(nullptr)
	{}

	RayPassMaterial RayPassMaterial::create(
			const types::Material *material,
			const RayPass *ray_pass)
	{
		auto result = RayPassMaterial();

		result._material = material;
		result._ray_pass = ray_pass;

		result._resource_declaration += util::f("struct MaterialResource", material->id(), "{\n");
		for (auto &resource : material->resources()) {
			if (resource.is_primitive()) {
				result._resource_declaration += "\t" + resource.declaration();
			}
		}
		result._resource_declaration += "};\n";

		return result;
	}
}
