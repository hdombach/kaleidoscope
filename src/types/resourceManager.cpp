#include "resourceManager.h"
#include "errors.h"
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

	vulkan::Texture const *ResourceManager::defaultTexture() const {
		return defaultTexture_;
	}

	vulkan::Texture const *ResourceManager::getTexture(const std::string &name) const {
		if (textures_.count(name)) {
			return textures_.at(name);
		}
		return defaultTexture_;
	}

	bool ResourceManager::hasTexture(const std::string &name) const {
		return textures_.count(name);
	}
}
