#pragma once

#include <string>
#include <unordered_map>

#include "../util/errors.hpp"
#include "../vulkan/Mesh.hpp"
#include "../util/result.hpp"
#include "../vulkan/Texture.hpp"


namespace types {
	/*
	 * The class containing all the scene resources such as 
	 * meshes, textures, and models
	 */
	class ResourceManager {
		public:
			ResourceManager();
			~ResourceManager();
	
			util::Result<void, KError> add_texture(
					std::string const &name,
					vulkan::Texture *texture);
			vulkan::Texture *default_texture();
			vulkan::Texture const *default_texture() const;
			vulkan::Texture *get_texture(std::string const &name);
			vulkan::Texture const *get_texture(std::string const &name) const;
			bool has_texture(std::string const &name) const;

			util::Result<void, KError> add_mesh(
					std::string const &name,
					vulkan::Mesh *mesh);
			vulkan::Mesh *default_mesh();
			vulkan::Mesh const *default_mesh() const;
			vulkan::Mesh *get_mesh(std::string const &name);
			vulkan::Mesh const *get_mesh(std::string const &name) const;
			bool has_mesh(std::string const &name) const;

		private:
			std::unordered_map<std::string, vulkan::Texture *> _textures;
			vulkan::Texture *_default_texture;

			std::unordered_map<std::string, vulkan::Mesh *> _meshes;
			vulkan::Mesh *_default_mesh;
	};
}
