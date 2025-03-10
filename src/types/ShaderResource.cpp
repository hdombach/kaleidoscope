#include <algorithm>
#include <string>
#include <stdlib.h>
#include <vector>

#include <glm/glm.hpp>

#include "ShaderResource.hpp"
#include "vulkan/MappedUniform.hpp"
#include "vulkan/Texture.hpp"
#include "util/format.hpp"
#include "util/KError.hpp"

namespace types {
	ShaderResource ShaderResource::create_primitive(
			std::string name,
			uint32_t val)
	{
		auto result = ShaderResource(name, Type::Uint);
		result._as_float = val;
		result._primitive_size = sizeof(uint32_t);
		result._declaration = util::f("uint ", name);
		result._uniform_declaration = result._glsl_declaration = "uint";
		result._alignment = 4;
		return result;

	}

	ShaderResource ShaderResource::create_primitive(
			std::string name,
			float val)
	{
		auto result = ShaderResource(name, Type::Float);
		result._as_float = val;
		result._primitive_size = sizeof(float);
		result._declaration = util::f("float ", name);
		result._uniform_declaration = result._glsl_declaration = "float";
		result._alignment = 4;
		return result;
	}

	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::mat4 mat)
	{
		auto result = ShaderResource(name, Type::Mat4);
		result._as_mat4 = mat;
		result._primitive_size = sizeof(mat);
		result._declaration = util::f("mat4 ", name);
		result._uniform_declaration = result._glsl_declaration = "mat4";
		result._alignment = 16;
		return result;
	}

	ShaderResource ShaderResource::create_primitive(
			std::string name,
			glm::vec3 vec)
	{
		auto result = ShaderResource(name, Type::Vec3);
		result._as_vec3 = vec;
		result._primitive_size = sizeof(vec);
		result._declaration = util::f("vec3 ", name);
		result._uniform_declaration = result._glsl_declaration = "vec3";
		result._alignment = 16;
		return result;
	}

ShaderResource ShaderResource::create_color(std::string name, glm::vec3 color) {
	auto result = create_primitive(name, color);
	result._type = Type::Color3;
	return result;
}

	ShaderResource ShaderResource::create_texture(
			std::string name,
			const vulkan::Texture *texture)
	{
		auto result = ShaderResource(name, Type::Texture);
		result._as_texture = texture;
		result._primitive_size = sizeof(uint32_t);
		result._declaration = util::f("sampler2D ", name);
		result._glsl_declaration = "sampler2D"; // reference to a list of textures
		result._uniform_declaration = "uint";
		result._alignment = 4;
		return result;
	}

	bool ShaderResource::is_primitive() const {
		switch (_type) {
			case Type::Uint:
			case Type::Mat4:
			case Type::Vec3:
			case Type::Float:
			case Type::Color3:
				return true;
			default:
				return false;
		}
	}

	cg::TemplObj ShaderResource::templ_declaration() const {
		return cg::TemplObj{
			{"uniform_type", _uniform_declaration},
			{"arg_type", _glsl_declaration},
			{"name", _name},
			{"is_texture", type() == Type::Texture}
		};
	}

	util::Result<void, KError> ShaderResource::set_texture(const vulkan::Texture *texture) {
		if (type() == Type::Texture) {
			if (texture != _as_texture) {
				_set_dirty_bit();
				_as_texture = texture;
			}
			return {};
		} else {
			return KError::invalid_arg("ShaderResource is not a texture");
		}
	}

	util::Result<vulkan::Texture const &, void> ShaderResource::as_texture() const {
		if (type() == Type::Texture) {
			return _as_texture;
		} else {
			return {};
		}
	}

	util::Result<void, KError> ShaderResource::set_mat4(glm::mat4 const &val) {
		if (type() == Type::Mat4) {
			if (val != _as_mat4) {
				_set_dirty_bit();
				_as_mat4 = val;
			}
			return {};
		} else {
			return KError::invalid_arg("ShaderResource is not a mat4");
		}
	}
	util::Result<glm::mat4, void> ShaderResource::as_mat4() const {
		if (type() == Type::Mat4) {
			return _as_mat4;
		} else {
			return {};
		}
	}

	util::Result<void, KError> ShaderResource::set_vec3(glm::vec3 const &val) {
		if (type() == Type::Vec3) {
			if (val != _as_vec3) {
				_set_dirty_bit();
				_as_vec3 = val;
			}
			return {};
		} else {
			return KError::invalid_arg("ShaderResource is not a vec3");
		}
	}

	util::Result<glm::vec3 const &, void> ShaderResource::as_vec3() const {
		if (type() == Type::Vec3) {
			return _as_vec3;
		} else {
			return {};
		}
	}

	util::Result<void, KError> ShaderResource::set_color3(const glm::vec3 &val) {
		if (type() == Type::Color3) {
			if (val != _as_vec3) {
				_set_dirty_bit();
				_as_vec3 = val;
			}
			return {};
		} else {
			return KError::invalid_arg("Shader Resource is not a color");
		}
	}

	util::Result<glm::vec3 const &, void> ShaderResource::as_color3() const {
		if (type() == Type::Color3) {
			return _as_vec3;
		} else {
			return {};
		}
	}

	util::Result<void, KError> ShaderResource::set_float(float val) {
		if (type() == Type::Float) {
			if (val != _as_float) {
				_set_dirty_bit();
				_as_float = val;
			}
			return {};
		} else {
			return KError::invalid_arg("ShaderResource is not a float");
		}
	}

	util::Result<float, void> ShaderResource::as_float() const {
		if (type() == Type::Float) {
			return _as_float;
		} else {
			return {};
		}
	}

	util::Result<void, KError> ShaderResource::set_uint32(uint32_t val) {
		if (type() == Type::Uint) {
			if (val != _as_uint32) {
				_set_dirty_bit();
				_as_uint32 = val;
			}
			return {};
		} else {
			return KError::invalid_arg("ShaderResource is not a float");
		}
	}

	util::Result<uint32_t, void> ShaderResource::as_uint32() const {
		if (type() == Type::Uint) {
			return _as_uint32;
		} else {
			return {};
		}
	}


	ShaderResource::ShaderResource(std::string &name, Type type):
		_name(name),
		_primitive_size(0),
		_type(type),
		_dirty_bit(false)
	{
	}

	void ShaderResource::_set_dirty_bit() {
		_dirty_bit = true;
	}

	ShaderResources::ShaderResources(ShaderResources const *parent):
		_resources(),
		_range(0),
		_parent(parent)
	{
		add_resource(ShaderResource::create_primitive("node_id", static_cast<uint32_t>(0)));
		add_resource(ShaderResource::create_primitive("object_transformation", glm::mat4(1.0)));
	}

	void ShaderResources::add_resource(ShaderResource resource) {
		for (auto &r : _resources) {
			if (r.name() == resource.name()) {
				return;
			}
		}
		_resources.push_back(resource);
		_range = _calc_range();
	}

	std::vector<ShaderResource const *> ShaderResources::get() const {
		auto result = std::vector<ShaderResource const *>();

		auto c = this;
		while (c) {
			for (auto &new_r : c->_resources) {
				bool found = false;
				for (auto &r : result) {
					if (r->name() == new_r.name()) {
						found = true;
					}
				}
				if (!found) {
					result.push_back(&new_r);
				}
			}
			c = c->_parent;
		}

		std::sort(result.begin(), result.end(), [](ShaderResource const *l, ShaderResource const *r) {
			return l->name() > r->name();
		});

		return result;
	}

	ShaderResource const *ShaderResources::get(std::string name) const {
		for (auto &r : _resources) {
			if (r.name() == name) {
				return &r;
			}
		}
		if (_parent) {
			return _parent->get(name);
		} else {
			return nullptr;
		}
	}

	cg::TemplObj ShaderResources::templ_declarations() const {
		auto result = cg::TemplList();
		for (auto &resource : get()) {
			result.push_back(resource->templ_declaration());
		}
		return result;
	}

	void ShaderResources::set_texture(
			const std::string &name,
			const vulkan::Texture *texture)
	{
		for (auto &resource : _resources) {
			if (resource.name() == name) {
				resource.set_texture(texture);
				return;
			}
		}
		add_resource(ShaderResource::create_texture(name, texture));
	}

	void ShaderResources::set_mat4(const std::string &name, const glm::mat4 &val) {
		for (auto &resource : _resources) {
			if (resource.name() == name) {
				resource.set_mat4(val);
				return;
			}
		}
		add_resource(ShaderResource::create_primitive(name, val));
	}

	void ShaderResources::set_vec3(std::string const &name, glm::vec3 const &val) {
		for (auto &resource : _resources) {
			if (resource.name() == name) {
				resource.set_vec3(val);
				return;
			}
		}
		add_resource(ShaderResource::create_primitive(name, val));
	}

	void ShaderResources::set_color3(const std::string &name, const glm::vec3 &val) {
		for (auto &resource : _resources) {
			if (resource.name() == name) {
				resource.set_color3(val);
				return;
			}
		}
		add_resource(ShaderResource::create_color(name, val));
	}

	void ShaderResources::set_float(const std::string &name, float &val) {
		for (auto &resource : _resources) {
			if (resource.name() == name) {
				resource.set_float(val);
				return;
			}
		}
		add_resource(ShaderResource::create_primitive(name, val));
	}

	void ShaderResources::set_uint32(const std::string &name, uint32_t &val) {
		for (auto &resource : _resources) {
			if (resource.name() == name) {
				resource.set_uint32(val);
				return;
			}
		}
		add_resource(ShaderResource::create_primitive(name, val));
	}

	util::Result<vulkan::Uniform, KError> ShaderResources::create_prim_uniform() const {
		size_t uniform_s = 0;
		for (auto resource : get()) {
			uniform_s += _calc_alignment(resource->alignment(), uniform_s);
			uniform_s += resource->primitive_size();
		}
		// Structs need to be aligned to 16 bytes
		uniform_s += _calc_alignment(16, uniform_s);

		auto uniform = vulkan::Uniform::create(uniform_s);
		TRY(uniform);
		update_prim_uniform(uniform.value());
		return {std::move(uniform.value())};
	}

	size_t ShaderResources::update_prim_uniform(vulkan::Uniform &uniform) const {
		return update_prim_uniform(uniform.raw_value());
	}

	size_t ShaderResources::update_prim_uniform(void *data) const {
		size_t cur_offset = 0;
		for (auto resource : get()) {
			cur_offset += _calc_alignment(resource->alignment(), cur_offset);
			auto value = static_cast<char *>(data) + cur_offset;
			if (resource->is_primitive()) {
				memcpy(value, resource->data(), resource->primitive_size());
			} else if (auto texture = resource->as_texture()) {
				uint32_t id = texture.value().id();
				memcpy(value, &id, sizeof(id));
			}
			cur_offset += resource->primitive_size();
		}
		cur_offset += _calc_alignment(16, cur_offset);
		return cur_offset;
	}

	bool ShaderResources::dirty_bits() const {
		bool res = false;
		for (auto &resource : _resources) {
			res |= resource.dirty_bit();
		}
		return res;
	}

	void ShaderResources::clear_dirty_bits() {
		for (auto &resource : _resources) {
			resource.clear_dirty_bit();
		}
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
		for (auto resource : get()) {
			result += _calc_alignment(resource->alignment(), result);
			result += resource->primitive_size();
		}
		result += _calc_alignment(16, result);
		return result;
	}
}
