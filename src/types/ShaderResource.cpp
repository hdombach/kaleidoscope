#include <stdlib.h>

#include <glm/glm.hpp>

#include "ShaderResource.hpp"

namespace types {
	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::mat4 &mat)
	{
		auto result = ShaderResource(name, Type::Mat4);
		result._primitive = &mat;
		result._primitive_size = sizeof(mat);
		return result;
	}

	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::vec3 &vec)
	{
		auto result = ShaderResource(name, Type::Vec3);
		result._primitive = &vec;
		result._primitive_size = sizeof(vec);
		return result;
	}

	ShaderResource ShaderResource::create_image(
			std::string name,
			const vulkan::ImageView &image_view)
	{
		auto result = ShaderResource(name, Type::Image);
		result._image_view = &image_view;
		return result;
	}

	bool ShaderResource::is_primitive() const {
		switch (_type) {
			case Type::Mat4:
			case Type::Vec3:
				return true;
			default:
				return false;
		}
	}

	ShaderResource::ShaderResource(std::string &name, Type type):
		_name(name),
		_image_view(nullptr),
		_primitive(nullptr),
		_primitive_size(0),
		_type(type)
	{
	}
}
