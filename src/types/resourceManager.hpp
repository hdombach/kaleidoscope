#pragma once

#include "../util/errors.hpp"
#include "../vulkan/material.hpp"
#include "../vulkan/mesh.hpp"
#include "../util/result.hpp"
#include "../vulkan/texture.hpp"
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
	
			util::Result<void, errors::TextureAlreadyExists> addTexture(
					std::string const &name,
					vulkan::Texture *texture);
			vulkan::Texture *defaultTexture();
			vulkan::Texture const *defaultTexture() const;
			vulkan::Texture const *getTexture(std::string const &name) const;
			bool hasTexture(std::string const &name) const;

			util::Result<void, errors::MeshAlreadyExists> addMesh(
					std::string const &name,
					vulkan::Mesh *mesh);
			vulkan::Mesh *defaultMesh();
			vulkan::Mesh const *defaultMesh() const;
			vulkan::Mesh const *getMesh(std::string const &name) const;
			bool hasMesh(std::string const &name) const;

			util::Result<void, errors::MaterialAlreadyExists> addMaterial(
					std::string const &name,
					vulkan::Material *material);
			vulkan::Material *defaultMaterial();
			vulkan::Material const *defaultMaterial() const;
			vulkan::Material const *getMaterial(std::string const &name) const;
			bool hasMaterial(std::string const &name) const;

		private:
			std::unordered_map<std::string, vulkan::Texture *> textures_;
			vulkan::Texture *defaultTexture_;

			std::unordered_map<std::string, vulkan::Mesh *> meshes_;
			vulkan::Mesh *defaultMesh_;

			std::unordered_map<std::string, vulkan::Material *> materials_;
			vulkan::Material *defaultMaterial_;
	};
}
