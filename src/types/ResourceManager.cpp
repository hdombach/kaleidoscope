#include "ResourceManager.hpp"
#include "../util/errors.hpp"
#include "../vulkan/Mesh.hpp"
#include "../util/result.hpp"
#include "../vulkan/StaticTexture.hpp"
#include "../vulkan/Texture.hpp"

namespace types {
	ResourceManager::ResourceManager() {
		//Can't use defaultTexture_ for holder because of const issues
		auto temp_texture = vulkan::StaticTexture::from_file("assets/default.png");
		add_texture("default", temp_texture.value());
		_default_texture = temp_texture.value();
	}

	ResourceManager::~ResourceManager() {
		for (auto texture : _textures) {
			delete texture.second;
		}
		for (auto mesh : _meshes) {
			delete mesh.second;
		}
	}

	util::Result<void, KError> ResourceManager::add_texture(
			const std::string &name,
			vulkan::Texture *texture)
	{
		if (_textures.count(name)) {
			return KError::texture_exists(name);
		}
		_textures[name] = texture;
		return {};
	}

	vulkan::Texture *ResourceManager::default_texture() {
		return _default_texture;
	}

	vulkan::Texture const *ResourceManager::default_texture() const {
		return _default_texture;
	}

	vulkan::Texture *ResourceManager::get_texture(const std::string &name) {
		if (has_texture(name)) {
			return _textures.at(name);
		}
		return _default_texture;
	}

	vulkan::Texture const *ResourceManager::get_texture(const std::string &name) const {
		if (has_texture(name)) {
			return _textures.at(name);
		}
		return _default_texture;
	}

	bool ResourceManager::has_texture(const std::string &name) const {
		return _textures.count(name);
	}

	util::Result<void, KError> ResourceManager::add_mesh(
			const std::string &name,
			vulkan::Mesh *mesh)
	{
		if (_meshes.count(name)) {
			return KError::mesh_already_exists(name);
		}
		_meshes[name] = mesh;
		return {};
	}

	vulkan::Mesh *ResourceManager::default_mesh() {
		return _default_mesh;
	}

	vulkan::Mesh const *ResourceManager::default_mesh() const {
		return _default_mesh;
	}

	vulkan::Mesh *ResourceManager::get_mesh(const std::string &name) {
		if (has_mesh(name)) {
			return _meshes.at(name);
		}
		return _default_mesh;
	}

	vulkan::Mesh const *ResourceManager::get_mesh(const std::string &name) const {
		if (has_mesh(name)) {
			return _meshes.at(name);
		}
		return _default_mesh;
	}

	bool ResourceManager::has_mesh(const std::string &name) const {
		return _meshes.count(name);
	}
}
