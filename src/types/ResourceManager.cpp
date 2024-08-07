
#include <algorithm>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "../util/file.hpp"
#include "../util/Util.hpp"
#include "../util/log.hpp"

#include "../vulkan/StaticTexture.hpp"
#include "../vulkan/Texture.hpp"
#include "../types/TextureMaterial.hpp"
#include "../types/ColorMaterial.hpp"

#include "../types/StaticMesh.hpp"
#include "../types/Mesh.hpp"

#include "ResourceManager.hpp"

namespace types {
	ResourceManager::ResourceManager() {
		//Can't use defaultTexture_ for holder because of const issues
		auto temp_texture = vulkan::StaticTexture::from_file(util::env_file_path("assets/default.png"));
		auto res = add_texture("default", temp_texture.value());
		if (res) {
			_default_texture = res.value();
		} else {
			LOG_FATAL_ERROR << "Couldn't create default texture" << std::endl;
		}
	}

	ResourceManager::~ResourceManager() {
		for (auto texture : _textures) {
			delete texture;
		}
	}

	util::Result<uint32_t, KError> ResourceManager::add_texture(
			const std::string &name,
			vulkan::Texture *texture)
	{
		if (_texture_map.count(name)) {
			return KError::texture_exists(name);
		}
		_textures.push_back(texture);
		_texture_map[name] = _textures.size() - 1;
		return {static_cast<uint32_t>(_textures.size() - 1)};
	}

	vulkan::Texture *ResourceManager::default_texture() {
		return _textures[_default_texture];
	}

	vulkan::Texture const *ResourceManager::default_texture() const {
		return _textures[_default_texture];
	}

	vulkan::Texture *ResourceManager::get_texture(const std::string &name) {
		if (has_texture(name)) {
			return _textures.at(_texture_map[name]);
		}
		return default_texture();
	}

	vulkan::Texture const *ResourceManager::get_texture(const std::string &name) const {
		if (has_texture(name)) {
			return _textures.at(_texture_map.at(name));
		}
		return default_texture();
	}

	bool ResourceManager::has_texture(const std::string &name) const {
		return _texture_map.count(name);
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_from_file(
			std::string const &name,
			std::string const &url)
	{
		auto res = StaticMesh::from_file(_get_mesh_id(), url);
		TRY(res);
		return _add_mesh(name, util::cast<Mesh, StaticMesh>(std::move(res.value())));
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_square(
			std::string const &name)
	{
		return _add_mesh(name, StaticMesh::create_square(_get_mesh_id()));
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_from_vertices(
			std::string const &name,
			std::vector<vulkan::Vertex> const &vertices)
	{
		return _add_mesh(name, StaticMesh::from_vertices(
					_get_mesh_id(),
					vertices));
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_from_vertices(
			std::string const &name,
			std::vector<vulkan::Vertex> const &vertices,
			std::vector<uint32_t> const &indices)
	{
		return _add_mesh(name, StaticMesh::from_vertices(
					_get_mesh_id(),
					vertices,
					indices));
	}

	Mesh *ResourceManager::default_mesh() {
		return _meshes[_default_mesh].get();
	}

	Mesh const *ResourceManager::default_mesh() const {
		return _meshes[_default_mesh].get();
	}

	Mesh *ResourceManager::update_mesh(const std::string &name) {
		//TODO: update mshes
		if (has_mesh(name)) {
			return _meshes[_mesh_map.at(name)].get();
		}
		return default_mesh();
	}

	Mesh const *ResourceManager::get_mesh(const std::string &name) const {
		if (has_mesh(name)) {
			return _meshes[_mesh_map.at(name)].get();
		}
		return default_mesh();
	}

	Mesh const *ResourceManager::get_mesh(uint32_t id) const {
		return _meshes[id].get();
	}

	bool ResourceManager::has_mesh(const std::string &name) const {
		return _mesh_map.count(name);
	}

	util::Result<void, KError> ResourceManager::add_mesh_observer(util::Observer *observer) {
		if (util::contains(_mesh_observers, observer)) {
			return KError::internal("Mesh observer already exists");
		}
		_mesh_observers.push_back(observer);
		for (auto &mesh : _meshes) {
			observer->obs_create(mesh->id());
		}
		return {};
	}

	util::Result<void, KError> ResourceManager::rem_mesh_observer(util::Observer *observer) {
		if (util::contains(_mesh_observers, observer)) {
			return KError::internal("Mesh observer does not exist");
		}
		std::ignore = std::remove(
				_mesh_observers.begin(),
				_mesh_observers.end(),
				observer);
		return {};
	}

	util::Result<uint32_t, KError> ResourceManager::add_texture_material(
			std::string const &name,
			vulkan::Texture *texture)
	{
		return _add_material(name, types::TextureMaterial::create(_get_material_id(), texture));
	}

	util::Result<uint32_t, KError> ResourceManager::add_color_material(
			std::string const &name,
			glm::vec3 color)
	{
		return _add_material(name, types::ColorMaterial::create(_get_material_id(), color));
	}


	types::Material const *ResourceManager::get_material(std::string const &name) const {
		if (has_material(name)) {
			return get_material(_material_map.at(name));
		}
		return nullptr;
	}

	types::Material const *ResourceManager::get_material(uint32_t id) const {
		if (id >= 0 && id < _materials.size()) {
			return _materials[id].get();
		}
		return nullptr;
	}

	bool ResourceManager::has_material(std::string const &name) const {
		return _material_map.count(name) > 0;
	}

	util::Result<void, KError> ResourceManager::add_material_observer(util::Observer *observer) {
		if (util::contains(_material_observers, observer)) {
			return KError::internal("Material observer already exists");
		}
		_material_observers.push_back(observer);
		for (auto &material : _materials) {
			observer->obs_create(material->id());
		}
		return {};
	}

	util::Result<void, KError> ResourceManager::rem_material_observer(util::Observer *observer) {
		if (util::contains(_material_observers, observer)) {
			return KError::internal("Material observer does not exist");
		}
		std::ignore = std::remove(
				_material_observers.begin(),
				_material_observers.end(),
				observer);
		return {};
	}

	util::Result<uint32_t, KError> ResourceManager::_add_mesh(
			const std::string &name,
			std::unique_ptr<Mesh> &&mesh)
	{
		if (_mesh_map.count(name)) {
			return KError::mesh_already_exists(name);
		}
		auto id = mesh->id();
		_meshes.push_back(std::move(mesh));
		_mesh_map[name] = id;
		for (auto &mesh_observer: _mesh_observers) {
			mesh_observer->obs_create(id);
		}
		return {id};
	}

	util::Result<uint32_t, KError> ResourceManager::_add_material(
			const std::string &name,
			std::unique_ptr<types::Material> &&material)
	{
		if (_material_map.count(name)) {
			return KError::material_already_exists(name);
		}
		auto id = material->id();
		_materials.push_back(std::move(material));
		_material_map[name] = id;
		for (auto &material_observer: _material_observers) {
			material_observer->obs_create(id);
		}
		return {id};
	}

	uint32_t ResourceManager::_get_mesh_id() {
		return _meshes.size();
	}

	uint32_t ResourceManager::_get_material_id() {
		return _materials.size();
	}
}
