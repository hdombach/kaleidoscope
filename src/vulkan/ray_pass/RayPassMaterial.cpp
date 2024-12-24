#include "RayPassMaterial.hpp"
#include "RayPass.hpp"
#include "types/Material.hpp"
#include "util/Util.hpp"
#include "util/errors.hpp"

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
	std::string const &RayPassMaterial::cg_struct_decl() {
		if (_cg_struct_decl.empty()) {
			_create_struct_decl();
		}
		return _cg_struct_decl;
	}
	std::string const &RayPassMaterial::cg_buf_decl() {
		if (_cg_buf_decl.empty()) {
			_create_buf_decl();
		}
		return _cg_buf_decl;
	}
	std::string const &RayPassMaterial::cg_frag_def() {
		if (_cg_frag_def.empty()) {
			_create_frag_def();
		}
		return _cg_frag_def;
	}
	std::string const &RayPassMaterial::cg_frag_call() {
		if (_cg_frag_call.empty()) {
			_create_frag_call();
		}
		return _cg_frag_call;
	}

	void RayPassMaterial::update() {
		_create_struct_decl();
	}

	void RayPassMaterial::_create_struct_decl() {
		_cg_struct_decl = util::f("struct Material", _material->id(), "{\n");
		for (auto &resource : _material->resources().get()) {
			if (resource->is_primitive()) {
				_cg_struct_decl += "\t" + resource->declaration() + ";\n";
			} else {
				_cg_struct_decl += "\tuint " + resource->name() + ";\n";
			}
		}

		if (_material->resources().range() < _ray_pass->max_material_range()) {
			auto len = (_ray_pass->max_material_range() - _material->resources().range()) / 4;
			_cg_struct_decl += "\tuint buf[" + std::to_string(len) + "];\n";
		}
		_cg_struct_decl += "};\n";
	}

	void RayPassMaterial::_create_buf_decl() {
		auto id = std::to_string(_material->id());
		_cg_buf_decl = "";

		_cg_buf_decl += "layout(binding = 7) readonly buffer material_buffer" + id + "{\n";

		_cg_buf_decl += "\tMaterial" + id + " material" + id + "[];\n";
		_cg_buf_decl += "};\n";
	}

	void RayPassMaterial::_create_frag_def() {
		auto id = std::to_string(_material->id());
		_cg_frag_def = "";

		_cg_frag_def += "void material" + id + "_main(inout vec4 outColor";
		_cg_frag_def += ", inout vec2 fragTexCoord";
		bool is_first = true;
		for (auto &resource : _material->resources().get()) {
			_cg_frag_def += ", in " + resource->declaration();
		}
		_cg_frag_def += ") {\n";
		
		{
			auto src = _material->frag_shader_src();
			util::indent(src, "\t");
			_cg_frag_def += src;
		}

		_cg_frag_def += "}\n";
	}

	void RayPassMaterial::_create_frag_call() {
		auto id = std::to_string(_material->id());
		auto textures = _ray_pass->used_textures();
		_cg_frag_call = "";

		_cg_frag_call += "material" + id + "_main(color, hit_info.uv";
		for (auto &resource : _material->resources().get()) {
			_cg_frag_call += ", ";
			if (resource->is_primitive()) {
				_cg_frag_call += "material" + id + "[hit_info.node_id]." + resource->name();
			} else if (resource->type() == types::ShaderResource::Type::Texture) {
				_cg_frag_call += "textures[material" + id + "[hit_info.node_id]." + resource->name() + "]";
			}
		}
		_cg_frag_call += ")";
	}
}
