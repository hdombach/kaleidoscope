#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <memory>

#include "util/Util.hpp"
#include "util/errors.hpp"
#include "util/result.hpp"
#include "util/Observer.hpp"
#include "util/UIDList.hpp"
#include "types/Mesh.hpp"
#include "vulkan/Texture.hpp"
#include "vulkan/Vertex.hpp"
#include "Material.hpp"


namespace types {
	/*
	 * The class containing all the scene resources such as 
	 * meshes, textures, and models
	 */
	class ResourceManager {
		public:
			using TextureContainer = util::UIDList<vulkan::Texture::Ptr, util::has_value, util::id_deref_trait>;

			using MeshContainer = util::UIDList<Mesh::Ptr, util::has_value, util::id_deref_trait>;

			using MaterialContainer = util::UIDList<Material::Ptr, util::has_value, util::id_deref_trait>;
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

			TextureContainer &textures() { return _textures; }
			TextureContainer const &textures() const { return _textures; }

			/*========================= Meshes =====================================*/
			util::Result<uint32_t, KError> add_mesh_from_file(
					std::string const &url);
			util::Result<uint32_t, KError> add_mesh_square(
					std::string const &name);
			util::Result<uint32_t, KError> add_mesh_from_vertices(
					std::string const &name,
					std::vector<vulkan::Vertex> const &vertices);
			util::Result<uint32_t, KError> add_mesh_mandelbulb(
					std::string const &name);
			util::Result<uint32_t, KError> add_mesh_mandelbox(
					std::string const &name);

			Mesh *default_mesh();
			Mesh const *default_mesh() const;
			Mesh const *get_mesh(std::string const &name) const;
			Mesh *get_mesh(std::string const &name);
			Mesh const *get_mesh(uint32_t id) const;
			Mesh *get_mesh(uint32_t id);
			bool has_mesh(std::string const &name) const;
			util::Result<void, KError> rename_mesh(uint32_t id, std::string const &name);

			MeshContainer &meshes() { return _meshes; }
			MeshContainer const &meshes() const { return _meshes; }

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
			types::Material *get_material(std::string const &name);
			types::Material const *get_material(uint32_t id) const;
			types::Material *get_material(uint32_t id);
			bool has_material(std::string const &name) const;
			util::Result<void, KError> rename_material(uint32_t id, std::string const &name);

			MaterialContainer &materials() { return _materials; }
			MaterialContainer const &materials() const { return _materials; }

			util::Result<void, KError> add_material_observer(util::Observer *observer);
			util::Result<void, KError> rem_material_observer(util::Observer *observer);

		private:
			util::Result<uint32_t, KError> _add_mesh(
					std::unique_ptr<Mesh> &&mesh);
			util::Result<uint32_t, KError> _add_material(
					std::string const &name,
					std::unique_ptr<types::Material> &&material);

		private:
			/**
			 * Containers
			 * id correspond to indexes in the container.
			 * The first element is always reserved for an unused object
			 * Elements can be nullptr if they are not created
			 */

			TextureContainer _textures;
			MeshContainer _meshes;
			MaterialContainer _materials;
			
			/**
			 * default indexes
			 * The index of default values
			 */
			uint32_t _default_texture;
			uint32_t _default_mesh;
			uint32_t _default_material;

			/**
			 * Observers
			 * Objects to be notified when containers change
			 * Does not own observers
			 */
			std::list<util::Observer *> _mesh_observers;
			std::list<util::Observer *> _material_observers;
	};
}
