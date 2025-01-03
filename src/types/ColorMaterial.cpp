#include <memory>

#include <glm/fwd.hpp>

#include "ColorMaterial.hpp"
#include "types/ShaderResource.hpp"

namespace types {
	std::unique_ptr<ColorMaterial> ColorMaterial::create(uint32_t id, glm::vec3 color) {
		auto result = std::unique_ptr<ColorMaterial>(new ColorMaterial());

		result->_id = id;
		result->_color = color;

		result->_resources.add_resource(types::ShaderResource::create_primitive(
					"position", result->_default_position));
		result->_resources.add_resource(types::ShaderResource::create_color(
					"color",
					result->_color));

		result->_frag_shader_src =
			"outColor = vec4(color, 1.0);\n";

		return result;
	}
}
