#include "resourceManager.h"
#include "errors.h"
#include "mesh.h"
#include "result.h"
#include "staticTexture.h"
#include "texture.h"

namespace types {
	ResourceManager::ResourceManager() {
		//Can't use defaultTexture_ for holder because of const issues
		auto tempTexture = vulkan::StaticTexture::fromFile("assets/default.png");
		addTexture("default", tempTexture.value());
		defaultTexture_ = tempTexture.value();
	}

	ResourceManager::~ResourceManager() {
		for (auto texture : textures_) {
			delete texture.second;
		}
		for (auto mesh : meshes_) {
			delete mesh.second;
		}
		for (auto material : materials_) {
			delete material.second;
		}
	}

	util::Result<void, errors::TextureAlreadyExists> ResourceManager::addTexture(
			const std::string &name,
			vulkan::Texture *texture)
	{
		if (textures_.count(name)) {
			return errors::TextureAlreadyExists{name};
		}
		textures_[name] = texture;
		return {};
	}

	vulkan::Texture *ResourceManager::defaultTexture() {
		return defaultTexture_;
	}

	vulkan::Texture const *ResourceManager::defaultTexture() const {
		return defaultTexture_;
	}

	vulkan::Texture const *ResourceManager::getTexture(const std::string &name) const {
		if (hasTexture(name)) {
			return textures_.at(name);
		}
		return defaultTexture_;
	}

	bool ResourceManager::hasTexture(const std::string &name) const {
		return textures_.count(name);
	}

	util::Result<void, errors::MeshAlreadyExists> ResourceManager::addMesh(
			const std::string &name,
			vulkan::Mesh *mesh)
	{
		if (meshes_.count(name)) {
			return errors::MeshAlreadyExists{name};
		}
		meshes_[name] = mesh;
		return {};
	}

	vulkan::Mesh *ResourceManager::defaultMesh() {
		return defaultMesh_;
	}

	vulkan::Mesh const *ResourceManager::defaultMesh() const {
		return defaultMesh_;
	}

	vulkan::Mesh const *ResourceManager::getMesh(const std::string &name) const {
		if (hasMesh(name)) {
			return meshes_.at(name);
		}
		return defaultMesh_;
	}

	bool ResourceManager::hasMesh(const std::string &name) const {
		return meshes_.count(name);
	}

	util::Result<void, errors::MaterialAlreadyExists> ResourceManager::addMaterial(
			const std::string &name,
			vulkan::Material *material)
	{
		if (hasMaterial(name)) {
			return errors::MaterialAlreadyExists{name};
		}
		materials_[name] = material;
		return {};
	}

	vulkan::Material const *ResourceManager::defaultMaterial() const {
		return defaultMaterial_;
	}

	vulkan::Material const *ResourceManager::getMaterial(const std::string &name) const {
		if (hasMaterial(name)) {
			return materials_.at(name);
		}
		return defaultMaterial_;
	}

	bool ResourceManager::hasMaterial(const std::string &name) const {
		return materials_.count(name);
	}
}
