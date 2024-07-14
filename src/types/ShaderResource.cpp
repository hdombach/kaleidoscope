#include <algorithm>
#include <stdlib.h>

#include <glm/glm.hpp>

#include "ShaderResource.hpp"
#include "../vulkan/MappedUniform.hpp"
#include "../util/format.hpp"

namespace types {
	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::mat4 &mat)
	{
		auto result = ShaderResource(name, Type::Mat4);
		result._primitive = &mat;
		result._primitive_size = sizeof(mat);
		result._declaration = util::f("\tmat4 ", name, ";\n");
		return result;
	}

	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::vec3 &vec)
	{
		auto result = ShaderResource(name, Type::Vec3);
		result._primitive = &vec;
		result._primitive_size = sizeof(vec);
		result._declaration = util::f("\tvec3 ", name, ";\n");
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

	util::Result<vulkan::Uniform, KError> ShaderResources::create_prim_uniform() const {
		int i = -1;
		size_t uniform_s = 0;
		for (auto &resource : _resources) {
			if (resource.is_primitive()) {
				uniform_s += resource.primitive_size();
				if (uniform_s % 16) {
					uniform_s += 16 - uniform_s % 16;
				}
			}
		}
		//uniform_s = (uniform_s / 16 + 1) * 16;

		auto uniform = vulkan::Uniform::create(uniform_s);
		TRY(uniform);
		update_prim_uniform(uniform.value());
		return {std::move(uniform.value())};
	}

	void ShaderResources::update_prim_uniform(vulkan::Uniform &uniform) const {
		update_prim_uniform(uniform, iterator(), iterator());
	}

	void ShaderResources::update_prim_uniform(
			vulkan::Uniform &uniform,
			const_iterator begin,
			const_iterator end) const
	{
		size_t cur_offset = 0;
		for (auto &resource : _resources) {
			if (resource.is_primitive()) {
				auto r = std::find_if(
						begin, end,
						[&resource](ShaderResource const &r) {
							return r.name() == resource.name();
						});

				auto value = static_cast<char *>(uniform.raw_value()) + cur_offset;
				if (r == end) {
					memcpy(value, resource.primitive(), resource.primitive_size());
				} else {
					memcpy(value, r->primitive(), resource.primitive_size());
				}

				cur_offset += resource.primitive_size();
				if (cur_offset % 16) {
					cur_offset += 16 - cur_offset % 16;
				}
				//cur_offset = ((cur_offset / 16) + 1) * 16;
			}
		}
	}
}
