
#include <algorithm>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "../util/file.hpp"
#include "../util/Util.hpp"
#include "../util/log.hpp"

#include "../vulkan/StaticTexture.hpp"
#include "../vulkan/Texture.hpp"
#include "../types/TextureMaterial.hpp"
#include "../types/CombTextureMaterial.hpp"
#include "../types/ColorMaterial.hpp"

#include "../types/StaticMesh.hpp"
#include "../types/Mesh.hpp"

#include "ResourceManager.hpp"

namespace types {
	ResourceManager::ResourceManager() {
		//Can't use defaultTexture_ for holder because of const issues
		auto res = add_texture_from_file("default", "assets/default.png");
		if (res) {
			_default_texture = res.value();
		} else {
			LOG_FATAL_ERROR << "Couldn't create default texture" << std::endl;
		}

		if (auto id = add_color_material("Default Material", glm::vec3(1.0, 0.0, 1.0))) {
			_default_material = id;
		} else {
			LOG_FATAL_ERROR << "Couldn't create default material" << std::endl;
		}
	}

	ResourceManager::~ResourceManager() {
		for (auto texture : _textures) {
			delete texture;
		}
	}

	util::Result<uint32_t, KError> ResourceManager::add_texture_from_file(
			const std::string &name,
			const std::string &url)
	{
		if (get_texture(name)) {
			return KError::texture_exists(name);
		}
		auto path = util::env_file_path(url);
		TRY(path);
		auto texture = vulkan::StaticTexture::from_file(
				_get_texture_id(),
				path.value());
		TRY(texture);
		_textures.push_back(texture.value());
		return {static_cast<uint32_t>(_textures.size() - 1)};
	}

	vulkan::Texture *ResourceManager::default_texture() {
		return _textures[_default_texture];
	}

	vulkan::Texture const *ResourceManager::default_texture() const {
		return _textures[_default_texture];
	}

	vulkan::Texture *ResourceManager::get_texture(const std::string &name) {
		for (auto t : _textures) {
			if (t && t->name() == name) {
				return t;
			}
		}
		return nullptr;
	}

	vulkan::Texture const *ResourceManager::get_texture(const std::string &name) const {
		for (auto t : _textures) {
			if (t && t->name() == name) {
				return t;
			}
		}
		return nullptr;
	}

	vulkan::Texture *ResourceManager::get_texture(uint32_t id) {
		if (_textures.size() > id) {
			return _textures[id];
		}
		return nullptr;
	}

	vulkan::Texture const *ResourceManager::get_texture(uint32_t id) const {
		if (_textures.size() > id) {
			return _textures[id];
		}
		return nullptr;
	}

	util::Result<void, KError> ResourceManager::rename_texture(uint32_t id, const std::string &name) {
		if (get_texture(name)) {
			return KError::name_already_exists(name);
		}
		get_texture(id)->set_name(name);
		return {};
	}

	ResourceManager::texture_iterator ResourceManager::texture_begin() {
		return texture_iterator(_textures.begin(), _textures.end(), util::ptr_exists<vulkan::Texture>);
	}
	ResourceManager::texture_iterator ResourceManager::texture_end() {
		return texture_iterator(_textures.end(), _textures.end(), util::ptr_exists<vulkan::Texture>);
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

	util::Result<uint32_t, KError> ResourceManager::add_comb_texture_material(
			std::string const &name,
			vulkan::Texture *prim_texture,
			vulkan::Texture *comb_texture)
	{
		return _add_material(name, types::CombTextureMaterial::create(
					_get_material_id(),
					prim_texture,
					comb_texture));
	}

	types::Material *ResourceManager::default_material() {
		return get_material(_default_material);
	}

	types::Material const *ResourceManager::default_material() const {
		return get_material(_default_material);
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

	types::Material *ResourceManager::get_material(uint32_t id) {
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

	uint32_t ResourceManager::_get_texture_id() {
		return _textures.size();
	}
}
