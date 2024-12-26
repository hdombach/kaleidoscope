#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <memory>

#include "util/errors.hpp"
#include "util/result.hpp"
#include "util/Observer.hpp"
#include "util/filter_iterator.hpp"
#include "util/IterAdapter.hpp"
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
			using TextureContainer = std::vector<std::unique_ptr<vulkan::Texture>>;
			using texture_iterator = util::filter_iterator<TextureContainer::iterator, util::exists<vulkan::Texture>>;
			using const_texture_iterator = util::filter_iterator<TextureContainer::const_iterator, util::exists<vulkan::Texture>>;

			using MeshContainer = std::vector<std::unique_ptr<Mesh>>;
			using mesh_iterator = util::filter_iterator<MeshContainer::iterator, util::exists<Mesh>>;
			using const_mesh_iterator = util::filter_iterator<MeshContainer::const_iterator, util::exists<Mesh>>;

			using MaterialContainer = std::vector<std::unique_ptr<Material>>;
			using material_iterator = util::filter_iterator<MaterialContainer::iterator, util::exists<Material>>;
			using const_material_iterator = util::filter_iterator<MaterialContainer::const_iterator, util::exists<Material>>;
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
			/**
			 * @brief An iterator that only contains valid textures
			 */
			auto textures() { return util::Adapt(texture_begin(), texture_end()); }
			const_texture_iterator texture_begin() const;
			const_texture_iterator texture_end() const;
			/**
			 * @brief A const iterator that only contains valid textures
			 */
			auto textures() const { return util::Adapt(texture_begin(), texture_end()); }
			/**
			 * @brief Underlying container containing unallocated textures as well
			 */
			TextureContainer const &texture_container() const;

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
			mesh_iterator mesh_begin();
			mesh_iterator mesh_end();
			/**
			 * @brief An iterator that only contains valid meshes
			 */
			auto meshes() { return util::Adapt(mesh_begin(), mesh_end()); }
			const_mesh_iterator mesh_begin() const;
			const_mesh_iterator mesh_end() const;
			/**
			 * @brief A const iterator that only contains valid meshes
			 */
			auto meshes() const { return util::Adapt(mesh_begin(), mesh_end()); }
			/**
			 * @brief Underlying container containg unallocated meshes as well
			 */
			MeshContainer const &mesh_container() const;

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
			material_iterator material_begin();
			material_iterator material_end();
			/**
			 * @brief An iterator that only contains valid materials
			 */
			auto materials() { return util::Adapt(material_begin(), material_end()); }
			const_material_iterator material_begin() const;
			const_material_iterator material_end() const;
			/**
			 * @brief A const iterator that only contains valid materials
			 */
			auto materials() const { return util::Adapt(material_begin(), material_end()); }
			/**
			 * @brief Underlying container containing unallocated materials as well
			 */
			MaterialContainer const &material_container() const;

			util::Result<void, KError> add_material_observer(util::Observer *observer);
			util::Result<void, KError> rem_material_observer(util::Observer *observer);

		private:
			util::Result<uint32_t, KError> _add_mesh(
					std::unique_ptr<Mesh> &&mesh);
			util::Result<uint32_t, KError> _add_material(
					std::string const &name,
					std::unique_ptr<types::Material> &&material);
			uint32_t _get_mesh_id();
			uint32_t _get_material_id();
			uint32_t _get_texture_id();

		private:
			/**
			 * Containers
			 * id correspond to indexes in the container.
			 * The first element is always reserved for an unused object
			 * Elements can be nullptr if they are not created
			 */

			std::vector<std::unique_ptr<vulkan::Texture>> _textures;
			MeshContainer _meshes;
			std::vector<std::unique_ptr<types::Material>> _materials;
			
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
