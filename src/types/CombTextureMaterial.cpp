#include "CombTextureMaterial.hpp"

namespace types {
	std::unique_ptr<CombTextureMaterial> CombTextureMaterial::create(
			uint32_t id,
			vulkan::Texture* prim_texture,
			vulkan::Texture* comb_texture)
	{
		auto result = std::unique_ptr<CombTextureMaterial>(new CombTextureMaterial());

		result->_prim_texture = prim_texture;
		result->_comb_texture = comb_texture;
		result->_comb_ratio = 0.75;
		result->_id = id;
		result->_object_transformation = glm::mat4(1.0);
		result->_resources.add_resource(
				ShaderResource::create_primitive("position", result->_default_position));
		result->_resources.add_resource(
				ShaderResource::create_primitive("object_transformation", result->_object_transformation));
		result->_resources.add_resource(
				ShaderResource::create_primitive("comb_ratio", result->_comb_ratio));
		result->_resources.add_resource(
				ShaderResource::create_image("primary_texture", prim_texture->image_view()));
		result->_resources.add_resource(
				ShaderResource::create_image("combine_texture", comb_texture->image_view()));

		result->_frag_shader_src =
			"outColor = texture(primary_texture, fragTexCoord) * (texture(combine_texture, fragTexCoord) * comb_ratio + (1 - comb_ratio));\n"
			"outColor.w = 1.0;\n"
			"outColor.xyz = vec3(pow(outColor.x, 1/2.2), pow(outColor.y, 1/2.2), pow(outColor.z, 1/2.2));\n";

		return result;
	}
}
