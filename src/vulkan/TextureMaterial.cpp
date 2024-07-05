#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/fwd.hpp>
#include <optional>
#include <utility>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <glm/vector_relational.hpp>

#include "TextureMaterial.hpp"
#include "DescriptorSet.hpp"
#include "defs.hpp"
#include "graphics.hpp"
#include "PreviewRenderPass.hpp"
#include "Shader.hpp"
#include "../util/file.hpp"

namespace vulkan {
	TextureMaterial::TextureMaterial(uint32_t id, Texture* texture):
	_texture(texture), _id(id)
	{
		_uniform.get()->object_transformation = glm::mat4(1.0);
		_resources.push_back(types::ShaderResource::create_uniform("object_uniform", _uniform));
		_resources.push_back(types::ShaderResource::create_image("texSampler", texture->image_view()));
	}

	TextureMaterial::~TextureMaterial() {
		LOG_MEMORY << "Destroying Texture Material" << std::endl;
	}
}
