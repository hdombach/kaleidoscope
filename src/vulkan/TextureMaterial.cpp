#include <vector>
#include <memory>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/fwd.hpp>

#include <vulkan/vulkan_core.h>
#include <glm/vector_relational.hpp>

#include "TextureMaterial.hpp"

namespace vulkan {
	std::unique_ptr<TextureMaterial> TextureMaterial::create(uint32_t id, Texture* texture) {
		auto result = std::unique_ptr<TextureMaterial>(new TextureMaterial());

		result->_texture = texture;
		result->_id = id;
		result->_object_transformation = glm::mat4(1.0);
		result->_resources.push_back(types::ShaderResource::create_primitive("object_transformation", result->_object_transformation));
		result->_resources.push_back(types::ShaderResource::create_image("texSampler", texture->image_view()));

		return result;
	}
}
