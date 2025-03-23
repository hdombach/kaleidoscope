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

	//Defer codegenerating to the last possible second
	//This can save situations like trying to 
	cg::TemplObj const &RayPassMaterial::cg_templobj() {
		if (_cg_templobj.type() == cg::TemplObj::Type::None) {
			_create_cg_templobj();
		}
		return _cg_templobj;
	}

	void RayPassMaterial::update() {
		_create_cg_templobj();
	}

	void RayPassMaterial::_create_cg_templobj() {

		auto declarations = cg::TemplList();
		for (auto &resource : _material->resources().get()) {
			declarations.push_back(resource->templ_declaration());
		}
		cg::TemplInt declaration_padding =
			(_ray_pass->max_material_range() - _material->resources().range()) / 4;

		_cg_templobj = cg::TemplObj{
			{"declarations", declarations},
			{"declaration_padding", declaration_padding},
			{"id", _material->id()},
			{"frag_src", _material->frag_shader_src()},
		};
	}
}
