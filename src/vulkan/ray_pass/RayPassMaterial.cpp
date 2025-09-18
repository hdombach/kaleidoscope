#include "RayPassMaterial.hpp"
#include "RayPass.hpp"
#include "types/Material.hpp"
#include "util/Util.hpp"
#include "util/KError.hpp"

namespace vulkan {
	RayPassMaterial::RayPassMaterial():
		_material(nullptr),
		_ray_pass(nullptr)
	{}

	util::Result<RayPassMaterial, KError> RayPassMaterial::create(
			const types::Material *material,
			const RayPass *ray_pass)
	{
		auto result = RayPassMaterial();

		if (material == nullptr) {
			return KError::invalid_arg("material is nullptr");
		}
		if (ray_pass == nullptr) {
			return KError::invalid_arg("material is nullptr");
		}

		result._material = material;
		result._ray_pass = ray_pass;

		return result;
	}

	void RayPassMaterial::update() { }
}
