#pragma once

#include "errors.h"
#include "result.h"
#include "texture.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


namespace types {
	/*
	 * The class containing all the scene resources such as 
	 * meshes, textures, and models
	 */
	class ResourceManager {
		public:
			ResourceManager();
			~ResourceManager();
	
			struct TextureAlreadyExists {};
			util::Result<void, errors::TextureAlreadyExists> addTexture(
					std::string const &name,
					vulkan::Texture *texture);
			vulkan::Texture *defaultTexture();
			vulkan::Texture const *defaultTexture() const;
			vulkan::Texture const *getTexture(std::string const &name) const;
			bool hasTexture(std::string const &name) const;

		private:
			std::unordered_map<std::string, vulkan::Texture *> textures_;
			vulkan::Texture const *defaultTexture_;
	};
}
