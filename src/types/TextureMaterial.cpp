#include <memory>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/fwd.hpp>

#include <vulkan/vulkan_core.h>
#include <glm/vector_relational.hpp>

#include "TextureMaterial.hpp"

namespace types {
	std::unique_ptr<TextureMaterial> TextureMaterial::create(uint32_t id, vulkan::Texture* texture) {
		auto result = std::unique_ptr<TextureMaterial>(new TextureMaterial());

		result->_texture = texture;
		result->_id = id;
		result->_object_transformation = glm::mat4(1.0);
		result->_resources.add_resource(types::ShaderResource::create_primitive("position", result->_default_position));
		result->_resources.add_resource(types::ShaderResource::create_primitive("object_transformation", result->_object_transformation));
		result->_resources.add_resource(types::ShaderResource::create_image("texSampler", texture->image_view()));

		result->_frag_shader_src =
			"outColor = texture(texSampler, fragTexCoord);\n"
			"outColor.xyz = vec3(pow(outColor.x, 1/2.2), pow(outColor.y, 1/2.2), pow(outColor.z, 1/2.2));\n";

		return result;
	}
}