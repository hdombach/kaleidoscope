#include <memory>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/vector_relational.hpp>

#include "TextureMaterial.hpp"
#include "vulkan/Texture.hpp"
#include "ShaderResource.hpp"

namespace types {
	std::unique_ptr<TextureMaterial> TextureMaterial::create(uint32_t id, vulkan::Texture* texture) {
		auto result = std::unique_ptr<TextureMaterial>(new TextureMaterial());

		result->_texture = texture;
		result->_id = id;
		result->_resources.add_resource(
				ShaderResource::create_primitive("position", result->_default_position));
		result->_resources.add_resource(
				ShaderResource::create_texture("primary_texture", texture));

		result->_frag_shader_src =
			"outColor = texture(primary_texture, fragTexCoord);\n"
			"outColor.w = 1.0;\n";

		return result;
	}
}
