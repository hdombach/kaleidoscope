#include "CombTextureMaterial.hpp"

#include <memory>

#include <glm/fwd.hpp>

#include "vulkan/Texture.hpp"
#include "types/ShaderResource.hpp"

namespace types {
	std::unique_ptr<CombTextureMaterial> CombTextureMaterial::create(
			uint32_t id,
			vulkan::Texture* prim_texture,
			vulkan::Texture* comb_texture)
	{
		auto result = std::unique_ptr<CombTextureMaterial>(new CombTextureMaterial());

		result->_prim_texture = prim_texture;
		result->_comb_texture = comb_texture;
		result->_id = id;
		result->_resources.add_resource(
				ShaderResource::create_primitive("position", glm::vec3()));
		result->_resources.add_resource(
				ShaderResource::create_primitive("comb_ratio", 0.75f));
		result->_resources.add_resource(
				ShaderResource::create_texture("primary_texture", prim_texture));
		result->_resources.add_resource(
				ShaderResource::create_texture("combine_texture", comb_texture));

		result->_frag_shader_src =
			"out_color = texture(primary_texture, in_uv) * (texture(combine_texture, in_uv) * comb_ratio + (1 - comb_ratio));\n"
			"out_color.w = 1.0;\n";

		return result;
	}
}
