#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <list>
#include <memory>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "../util/Observer.hpp"
#include "../util/filter_iterator.hpp"
#include "../types/Mesh.hpp"
#include "../vulkan/Texture.hpp"
#include "../vulkan/Vertex.hpp"
#include "Material.hpp"


namespace types {
	/*
	 * The class containing all the scene resources such as 
	 * meshes, textures, and models
	 */
	class ResourceManager {
		public:
			using TextureContainer = std::vector<vulkan::Texture *>;
			using texture_iterator = TextureContainer::iterator;

			using MeshContainer = std::vector<std::unique_ptr<Mesh>>;
			using mesh_iterator = MeshContainer::iterator;
		public:
			ResourceManager();
			~ResourceManager();
	
			/*=========================== Textures =================================*/
			util::Result<uint32_t, KError> add_texture_from_file(
					std::string const &url);

			vulkan::Texture *default_texture();
			vulkan::Texture const *default_texture() const;
			vulkan::Texture *get_texture(std::string const &name);
			vulkan::Texture const *get_texture(std::string const &name) const;
			vulkan::Texture *get_texture(uint32_t id);
			vulkan::Texture const *get_texture(uint32_t id) const;
			util::Result<void, KError> rename_texture(uint32_t id, std::string const &name);
			texture_iterator texture_begin();
			texture_iterator texture_end();

			/*========================= Meshes =====================================*/
			util::Result<uint32_t, KError> add_mesh_from_file(
					std::string const &name,
					std::string const &url);
			util::Result<uint32_t, KError> add_mesh_square(
					std::string const &name);
			util::Result<uint32_t, KError> add_mesh_from_vertices(
					std::string const &name,
					std::vector<vulkan::Vertex> const &vertices);

			Mesh *default_mesh();
			Mesh const *default_mesh() const;
			Mesh const *get_mesh(std::string const &name) const;
			Mesh *get_mesh(std::string const &name);
			Mesh const *get_mesh(uint32_t id) const;
			Mesh *get_mesh(uint32_t id);
			bool has_mesh(std::string const &name) const;
			util::Result<void, KError> rename_mesh(uint32_t id, std::string const &name);
			mesh_iterator mesh_begin();
			mesh_iterator mesh_end();

			util::Result<void, KError> add_mesh_observer(util::Observer *observer);
			util::Result<void, KError> rem_mesh_observer(util::Observer *observer);

			/*========================= Materials ==================================*/
			util::Result<uint32_t, KError> add_texture_material(
					std::string const &name,
					vulkan::Texture *texture);
			util::Result<uint32_t, KError> add_color_material(std::string const &name, glm::vec3 color);
			util::Result<uint32_t, KError> add_comb_texture_material(
					std::string const &name,
					vulkan::Texture *prim_texture,
					vulkan::Texture *comb_texture);

			types::Material *default_material();
			types::Material const *default_material() const;
			types::Material const *get_material(std::string const &name) const;
			types::Material const *get_material(uint32_t id) const;
			types::Material *get_material(uint32_t id);
			bool has_material(std::string const &name) const;

			util::Result<void, KError> add_material_observer(util::Observer *observer);
			util::Result<void, KError> rem_material_observer(util::Observer *observer);

		private:
			util::Result<uint32_t, KError> _add_mesh(
					std::string const &name,
					std::unique_ptr<Mesh> &&mesh);
			util::Result<uint32_t, KError> _add_material(
					std::string const &name,
					std::unique_ptr<types::Material> &&material);
			uint32_t _get_mesh_id();
			uint32_t _get_material_id();
			uint32_t _get_texture_id();

			std::vector<vulkan::Texture *> _textures;
			uint32_t _default_texture;

			MeshContainer _meshes;
			std::unordered_map<std::string, uint32_t> _material_map;
			std::vector<std::unique_ptr<types::Material>> _materials;
			uint32_t _default_mesh;
			uint32_t _default_material;
			std::list<util::Observer *> _mesh_observers;
			std::list<util::Observer *> _material_observers;
	};
}
