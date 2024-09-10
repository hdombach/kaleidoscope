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

		result._create_declaration();
		result._create_buf_stuff();
		result._create_frag_src();
		result._create_frag_call();

		return result;
	}

	void RayPassMaterial::update() {
		_create_frag_call();
		_create_declaration();
	}

	void RayPassMaterial::_create_declaration() {
		_resource_declaration = util::f("struct Material", _material->id(), "{\n");
		for (auto &resource : _material->resources()) {
			if (resource.is_primitive()) {
				_resource_declaration += "\t" + resource.declaration() + ";\n";
			}
		}

		if (_material->resources().range() < _ray_pass->max_material_range()) {
			auto len = (_ray_pass->max_material_range() - _material->resources().range()) / 4;
			_resource_declaration += "\tuint buf[" + std::to_string(len) + "];\n";
		}
		_resource_declaration += "};\n";
	}

	void RayPassMaterial::_create_buf_stuff() {
		auto id = std::to_string(_material->id());
		_material_buf = "";

		_material_buf += "layout(binding = 6) readonly buffer material_buffer" + id + "{\n";

		_material_buf += "\tMaterial" + id + " material" + id + "[];\n";
		_material_buf += "};\n";
	}

	void RayPassMaterial::_create_frag_src() {
		auto id = std::to_string(_material->id());
		_frag_src = "";

		_frag_src += "void material" + id + "_main(inout vec4 outColor";
		_frag_src += ", inout vec2 fragTexCoord";
		bool is_first = true;
		for (auto &resource : _material->resources()) {
			_frag_src += ", in " + resource.declaration();
		}
		_frag_src += ") {\n";
		
		{
			auto src = _material->frag_shader_src();
			util::indent(src, "\t");
			_frag_src += src;
		}

		_frag_src += "}\n";
	}

	void RayPassMaterial::_create_frag_call() {
		auto id = std::to_string(_material->id());
		auto textures = _ray_pass->used_textures();
		_frag_call = "";

		_frag_call += "material" + id + "_main(color, uv";
		for (auto &resource : _material->resources()) {
			_frag_call += ", ";
			if (resource.is_primitive()) {
				_frag_call += "material" + id + "[id]." + resource.name();
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
				_frag_call += "textures[" + std::to_string(i) + "]";
			}
		}
		_frag_call += ")";
	}
}
