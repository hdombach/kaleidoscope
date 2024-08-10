#include <algorithm>
#include <stdlib.h>

#include <glm/glm.hpp>

#include "ShaderResource.hpp"
#include "../vulkan/MappedUniform.hpp"
#include "../util/format.hpp"

namespace types {
	ShaderResource ShaderResource::create_primitive(
			std::string name,
			float &val)
	{
		auto result = ShaderResource(name, Type::Float);
		result._primitive = &val;
		result._primitive_size = sizeof(float);
		result._declaration = util::f("float ", name);
		result._alignment = 4;
		return result;
	}

	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::mat4 &mat)
	{
		auto result = ShaderResource(name, Type::Mat4);
		result._primitive = &mat;
		result._primitive_size = sizeof(mat);
		result._declaration = util::f("mat4 ", name);
		result._alignment = 16;
		return result;
	}

	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::vec3 &vec)
	{
		auto result = ShaderResource(name, Type::Vec3);
		result._primitive = &vec;
		result._primitive_size = sizeof(vec);
		result._declaration = util::f("vec3 ", name);
		result._alignment = 16;
		return result;
	}

	ShaderResource ShaderResource::create_image(
			std::string name,
			const vulkan::ImageView &image_view)
	{
		auto result = ShaderResource(name, Type::Image);
		result._image_view = &image_view;
		result._alignment = 0;
		result._declaration = util::f("sampler2D ", name);
		return result;
	}

	bool ShaderResource::is_primitive() const {
		switch (_type) {
			case Type::Mat4:
			case Type::Vec3:
			case Type::Float:
				return true;
			default:
				return false;
		}
	}

	util::Result<glm::mat4&, void> ShaderResource::as_mat4() {
		if (type() == Type::Mat4) {
			return static_cast<glm::mat4*>(_primitive);
		} else {
			return {};
		}
	}

	util::Result<glm::vec3&, void> ShaderResource::as_vec3() {
		if (type() == Type::Vec3) {
			return static_cast<glm::vec3*>(_primitive);
		} else {
			return {};
		}
	}

	util::Result<float&, void> ShaderResource::as_float() {
		if (type() == Type::Float) {
			return static_cast<float*>(_primitive);
		} else {
			return {};
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

	void ShaderResources::add_resource(ShaderResource resource) {
		_resources.push_back(resource);
		_range = _calc_range();
	}

	util::Result<ShaderResource&, void> ShaderResources::get(std::string name) {
		auto res = std::find_if(begin(), end(), [&name](ShaderResource &r) {
				return r.name() == name;
		});
		if (res == end()) {
			return {};
		} else {
			return res.base();
		}
	}

	util::Result<ShaderResource const&, void> ShaderResources::get(std::string name) const {
		auto res = std::find_if(begin(), end(), [&name](ShaderResource const &r) {
				return r.name() == name;
		});
		if (res == end()) {
			return {};
		} else {
			return res.base();
		}
	}

	bool ShaderResources::contains(std::string name) const {
		for (auto &resource : *this) {
			if (resource.name() == name) {
				return true;
			}
		}
		return false;
	}

	util::Result<vulkan::Uniform, KError> ShaderResources::create_prim_uniform() const {
		int i = -1;
		size_t uniform_s = 0;
		for (auto &resource : _resources) {
			if (resource.is_primitive()) {
				uniform_s += resource.primitive_size();
				uniform_s += _calc_alignment(resource.alignment(), uniform_s);
			}
		}
		// Structs need to be aligned to 16 bytes
		uniform_s += _calc_alignment(16, uniform_s);

		auto uniform = vulkan::Uniform::create(uniform_s);
		TRY(uniform);
		update_prim_uniform(uniform.value());
		return {std::move(uniform.value())};
	}

	size_t ShaderResources::update_prim_uniform(vulkan::Uniform &uniform) const {
		return update_prim_uniform(uniform, iterator(), iterator());
	}

	size_t ShaderResources::update_prim_uniform(
			vulkan::Uniform &uniform,
			const_iterator begin,
			const_iterator end) const
	{
		return update_prim_uniform(uniform.raw_value(), begin, end);
	}

	size_t ShaderResources::update_prim_uniform(
			void *data,
			const_iterator begin,
			const_iterator end) const
	{
		size_t cur_offset = 0;
		for (auto &resource : _resources) {
			if (resource.is_primitive()) {
				auto r = std::find_if(
						begin, end,
						[&resource](ShaderResource const &r) {
							//TODO: maybe throw a error if a type exists with different type
							return r.name() == resource.name() && r.type() == resource.type();
						});

				auto value = static_cast<char *>(data) + cur_offset;
				if (r == end) {
					memcpy(value, resource.primitive(), resource.primitive_size());
				} else {
					memcpy(value, r->primitive(), resource.primitive_size());
				}

				cur_offset += resource.primitive_size();
				cur_offset += _calc_alignment(resource.alignment(), cur_offset);
			}
		}
		cur_offset += _calc_alignment(16, cur_offset);
		return cur_offset;

	}

	size_t ShaderResources::_calc_alignment(size_t alignment, size_t cur_offset) {
		if (cur_offset % alignment) {
			return alignment - cur_offset % alignment;
		} else {
			return 0;
		}
	}

	size_t ShaderResources::_calc_range() const {
		size_t result = 0;
		for (auto &resource : _resources) {
			if (resource.is_primitive()) {
				result += resource.primitive_size();
				result += _calc_alignment(resource.alignment(), result);
			}
		}
		return result;
	}
}
