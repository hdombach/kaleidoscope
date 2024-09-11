#include "RayPassMaterial.hpp"
#include "../types/Material.hpp"
#include "RayPass.hpp"
#include "../util/Util.hpp"

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
		_create_frag_call();
		_create_struct_decl();
	}

	void RayPassMaterial::_create_struct_decl() {
		_cg_struct_decl = util::f("struct Material", _material->id(), "{\n");
		for (auto &resource : _material->resources()) {
			if (resource.is_primitive()) {
				_cg_struct_decl += "\t" + resource.declaration() + ";\n";
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

		_cg_buf_decl += "layout(binding = 6) readonly buffer material_buffer" + id + "{\n";

		_cg_buf_decl += "\tMaterial" + id + " material" + id + "[];\n";
		_cg_buf_decl += "};\n";
	}

	void RayPassMaterial::_create_frag_def() {
		auto id = std::to_string(_material->id());
		_cg_frag_def = "";

		_cg_frag_def += "void material" + id + "_main(inout vec4 outColor";
		_cg_frag_def += ", inout vec2 fragTexCoord";
		bool is_first = true;
		for (auto &resource : _material->resources()) {
			_cg_frag_def += ", in " + resource.declaration();
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

		_cg_frag_call += "material" + id + "_main(color, uv";
		for (auto &resource : _material->resources()) {
			_cg_frag_call += ", ";
			if (resource.is_primitive()) {
				_cg_frag_call += "material" + id + "[node_id]." + resource.name();
			} else if (resource.type() == types::ShaderResource::Type::Image) {
				int i = 0;
				for (auto texture : textures) {
					if (resource.as_image().value().value() == texture) {
						break;
					}
					i++;
				}
				if (i == textures.size()) {
					LOG_ERROR << "Texture not found for ray pass" << std::endl;
				}
				_cg_frag_call += "textures[" + std::to_string(i) + "]";
			}
		}
		_cg_frag_call += ")";
	}
}
