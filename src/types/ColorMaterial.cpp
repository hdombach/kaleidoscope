#include <vector>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include "ColorMaterial.hpp"

namespace types {
	std::unique_ptr<ColorMaterial> ColorMaterial::create(uint32_t id, glm::vec3 color) {
		auto result = std::unique_ptr<ColorMaterial>(new ColorMaterial());

		result->_id = id;
		result->_color = color;
		result->_object_transformation = glm::mat4(1.0);

		result->_resources.add_resource(types::ShaderResource::create_primitive(
					"object_transformation",
					result->_object_transformation));

		result->_resources.add_resource(types::ShaderResource::create_primitive(
					"color",
					result->_color));

		return result;
	}
}
