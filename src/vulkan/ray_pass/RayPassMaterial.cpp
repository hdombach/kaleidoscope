#include "RayPassMaterial.hpp"
#include "RayPass.hpp"
#include "types/Material.hpp"
#include "util/Util.hpp"

namespace vulkan {
	RayPassMaterial::RayPassMaterial():
		_material(nullptr),
		_ray_pass(nullptr)
	{}

	util::Result<RayPassMaterial, RayPassMaterial::Error> RayPassMaterial::create(
			const types::Material *material,
			const RayPass *ray_pass)
	{
		auto result = RayPassMaterial();

		if (material == nullptr) {
			return Error(ErrorType::INVALID_ARG, "Material is nullptr");
		}
		if (ray_pass == nullptr) {
			return Error(ErrorType::INVALID_ARG, "Raypass is nullptr");
		}

		result._material = material;
		result._ray_pass = ray_pass;

		return result;
	}

	void RayPassMaterial::update() { }
}

template<>
const char *vulkan::RayPassMaterial::Error::type_str(vulkan::RayPassMaterial::ErrorType t) {
	switch (t) {
		case vulkan::RayPassMaterial::ErrorType::MISC:
			return "RayPassMaterial.MISC";
		case vulkan::RayPassMaterial::ErrorType::INVALID_ARG:
			return "RayPassMaterial.ARG";
	}
}
