#include <cstdlib>
#include <stdlib.h>

#include "ShaderResource.hpp"

namespace types {
	ShaderResource ShaderResource::create_image(
			std::string name,
			const vulkan::ImageView &image_view)
	{
		auto result = ShaderResource(name);
		result._image_view = &image_view;
		return result;
	}

	ShaderResource ShaderResource::create_image_target(
			std::string name,
			const vulkan::ImageView &image_view)
	{
		auto result = ShaderResource(name);
		result._image_view = &image_view;
		return result;
	}

	template<typename T>
		ShaderResource ShaderResource::create_storage_buffer(
				std::string name,
				std::vector<vulkan::VType<T>> &buffer)
		{
			auto result = ShaderResource(name);
			result._objects = buffer[0].get();
			result._object_size = sizeof(T);
			result._object_count = buffer.size();
		}

	size_t ShaderResource::range() const {
		return _object_size * _object_count;
	}

	void const *ShaderResource::objects() const {
		return _objects;
	}

	size_t ShaderResource::object_size() const {
		return _object_size;
	}

	size_t ShaderResource::object_count() const {
		return _object_count;
	}

	ShaderResource::Type ShaderResource::type() const {
		return _type;
	}

	vulkan::ImageView const &ShaderResource::image_view() const {
		return *_image_view;
	}

	ShaderResource::~ShaderResource() { }

	ShaderResource::ShaderResource(std::string &name):
		_name(name),
		_objects(nullptr),
		_object_size(0),
		_object_count(0),
		_image_view(nullptr)
	{
	}
}
