#include <vector>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include "ColorMaterial.hpp"
#include "defs.hpp"
#include "Shader.hpp"

namespace vulkan {
	ColorMaterial::ColorMaterial(uint32_t id, glm::vec3 color):
		_id(id),
		_color(color)
	{
		_resources.push_back(types::ShaderResource::create_uniform("uniform_buffer", _uniform));
	}

	std::vector<types::ShaderResource> const &ColorMaterial::resources() const {
		return _resources;
	}	

	uint32_t ColorMaterial::id() const {
		return _id;
	}
}
