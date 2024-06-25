#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "../util/Observer.hpp"
#include "../types/Mesh.hpp"
#include "../vulkan/Texture.hpp"
#include "../vulkan/Vertex.hpp"
#include "../vulkan/Material.hpp"


namespace types {
	/*
	 * The class containing all the scene resources such as 
	 * meshes, textures, and models
	 */
	class ResourceManager {
		public:
			ResourceManager();
			~ResourceManager();
	
			util::Result<uint32_t, KError> add_texture(
					std::string const &name,
					vulkan::Texture *texture);

			vulkan::Texture *default_texture();
			vulkan::Texture const *default_texture() const;
			vulkan::Texture *get_texture(std::string const &name);
			vulkan::Texture const *get_texture(std::string const &name) const;
			bool has_texture(std::string const &name) const;

			util::Result<uint32_t, KError> add_mesh_from_file(
					std::string const &name,
					std::string const &url);
			util::Result<uint32_t, KError> add_mesh_square(
					std::string const &name);
			util::Result<uint32_t, KError> add_mesh_from_vertices(
					std::string const &name,
					std::vector<vulkan::Vertex> const &vertices);
			util::Result<uint32_t, KError> add_mesh_from_vertices(
					std::string const &name,
					std::vector<vulkan::Vertex> const &vertices,
					std::vector<uint32_t> const &indices);

			util::Result<uint32_t, KError> add_texture_material(
					std::string const &name,
					vulkan::Texture *texture);

			Mesh *default_mesh();
			Mesh const *default_mesh() const;
			Mesh *update_mesh(std::string const &name);
			Mesh const *get_mesh(std::string const &name) const;
			Mesh const *get_mesh(uint32_t id) const;
			bool has_mesh(std::string const &name) const;

			util::Result<void, KError> add_mesh_observer(util::Observer *observer);
			util::Result<void, KError> rem_mesh_observer(util::Observer *observer);

			util::Result<void, KError> add_material_observer(util::Observer *observer);
			util::Result<void, KError> rem_material_observer(util::Observer *observer);

		private:
			util::Result<uint32_t, KError> _add_mesh(
					std::string const &name,
					Mesh *mesh);
			util::Result<uint32_t, KError> _add_material(
					std::string const &name,
					vulkan::Material *material);
			uint32_t _get_mesh_id();
			uint32_t _get_material_id();

			std::unordered_map<std::string, uint32_t> _texture_map;
			std::vector<vulkan::Texture *> _textures;
			uint32_t _default_texture;

			std::unordered_map<std::string, uint32_t> _mesh_map;
			std::vector<Mesh *> _meshes;
			std::unordered_map<std::string, uint32_t> _material_map;
			std::vector<vulkan::Material *> _materials;
			uint32_t _default_mesh;
			std::list<util::Observer *> _mesh_observers;
			std::list<util::Observer *> _material_observers;
	};
}
