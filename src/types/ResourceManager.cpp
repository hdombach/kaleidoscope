
#include <algorithm>
#include <string>

#include "util/errors.hpp"
#include "util/result.hpp"
#include "util/file.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"

#include "vulkan/StaticTexture.hpp"
#include "vulkan/Texture.hpp"
#include "types/TextureMaterial.hpp"
#include "types/CombTextureMaterial.hpp"
#include "types/ColorMaterial.hpp"

#include "types/MandelbulbMesh.hpp"
#include "types/MandelboxMesh.hpp"
#include "types/StaticMesh.hpp"
#include "types/Mesh.hpp"

#include "ResourceManager.hpp"

namespace types {
	ResourceManager::ResourceManager() {
		_textures.push_back(nullptr);
		_meshes.push_back(nullptr);
		_materials.push_back(nullptr);
		//Can't use defaultTexture_ for holder because of const issues
		auto res = add_texture_from_file("assets/default.png");
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

		if (auto id = add_mesh_square("Default mesh")) {
			_default_mesh = id;
		} else {
			LOG_FATAL_ERROR << "Couldn't create default mesh" << std::endl;
		}
	}

	ResourceManager::~ResourceManager() {
		_textures.clear();
	}

	util::Result<uint32_t, KError> ResourceManager::add_texture_from_file(
			const std::string &url)
	{
		auto path = util::env_file_path(url);
		TRY(path);
		auto texture = vulkan::StaticTexture::from_file(
				_get_texture_id(),
				path.value());
		TRY(texture);
		int i = 1;
		auto base_name = texture.value()->name();
		while (get_texture(texture.value()->name())) {
			texture.value()->set_name(base_name + "_" + std::to_string(i));
			i++;
		}
		_textures.push_back(std::move(texture.value()));
		return {static_cast<uint32_t>(_textures.size() - 1)};
	}

	vulkan::Texture *ResourceManager::default_texture() {
		return _textures[_default_texture].get();
	}

	vulkan::Texture const *ResourceManager::default_texture() const {
		return _textures[_default_texture].get();
	}

	vulkan::Texture *ResourceManager::get_texture(const std::string &name) {
		for (auto &t : textures()) {
			if (t->name() == name) {
				return t.get();
			}
		}
		return nullptr;
	}

	vulkan::Texture const *ResourceManager::get_texture(const std::string &name) const {
		for (auto &t : textures()) {
			if (t->name() == name) {
				return t.get();
			}
		}
		return nullptr;
	}

	vulkan::Texture *ResourceManager::get_texture(uint32_t id) {
		if (_textures.size() > id) {
			return _textures[id].get();
		}
		return nullptr;
	}

	vulkan::Texture const *ResourceManager::get_texture(uint32_t id) const {
		if (_textures.size() > id) {
			return _textures[id].get();
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
		return texture_iterator(_textures.begin(), _textures.end(), util::exists<vulkan::Texture>);
	}
	ResourceManager::texture_iterator ResourceManager::texture_end() {
		return texture_iterator(_textures.end(), _textures.end(), util::exists<vulkan::Texture>);
	}
	ResourceManager::const_texture_iterator ResourceManager::texture_begin() const {
		return const_texture_iterator(_textures.begin(), _textures.end(), util::exists<vulkan::Texture>);
	}
	ResourceManager::const_texture_iterator ResourceManager::texture_end() const {
		return const_texture_iterator(_textures.end(), _textures.end(), util::exists<vulkan::Texture>);
	}
	ResourceManager::TextureContainer const &ResourceManager::texture_container() const {
		return _textures;
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_from_file(
			std::string const &url)
	{
		auto path = util::env_file_path(url);
		TRY(path);
		auto mesh = StaticMesh::from_file(_get_mesh_id(), path.value());
		TRY(mesh);
		int i = 1;
		auto base_name = mesh.value()->name();
		while (get_mesh(mesh.value()->name())) {
			mesh.value()->set_name(base_name + "_" + std::to_string(i));
			i++;
		}

		return _add_mesh(std::move(mesh.value()));
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_square(
			std::string const &name)
	{
		return _add_mesh(StaticMesh::create_square(name, _get_mesh_id()));
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_from_vertices(
			std::string const &name,
			std::vector<vulkan::Vertex> const &vertices)
	{
		return _add_mesh(StaticMesh::from_vertices(
					name,
					_get_mesh_id(),
					vertices));
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_mandelbulb(
			std::string const &name) 
	{
		return _add_mesh(MandelbulbMesh::create(name, _get_mesh_id()));
	}

	util::Result<uint32_t, KError> ResourceManager::add_mesh_mandelbox(
		std::string const &name)
		{
			return _add_mesh(MandelboxMesh::create(name, _get_mesh_id()));
		}

	Mesh *ResourceManager::default_mesh() {
		return _meshes[_default_mesh].get();
	}

	Mesh const *ResourceManager::default_mesh() const {
		return _meshes[_default_mesh].get();
	}

	Mesh const *ResourceManager::get_mesh(const std::string &name) const {
		for (auto &m : meshes()) {
			if (m->name() == name) {
				return m.get();
			}
		}
		return nullptr;
	}

	Mesh *ResourceManager::get_mesh(const std::string &name) {
		for (auto &m : meshes()) {
			if (m->name() == name) {
				return m.get();
			}
		}
		return nullptr;
	}


	Mesh const *ResourceManager::get_mesh(uint32_t id) const {
		if (_meshes.size() > id) {
			return _meshes[id].get();
		}
		return nullptr;
	}

	Mesh *ResourceManager::get_mesh(uint32_t id) {
		if (_meshes.size() > id) {
			return _meshes[id].get();
		}
		return nullptr;
	}

	bool ResourceManager::has_mesh(const std::string &name) const {
		for (auto &m : meshes()) {
			if (m->name() == name) {
				return true;
			}
		}
		return false;
	}

	util::Result<void, KError> ResourceManager::rename_mesh(
			uint32_t id,
			const std::string &name)
	{
		if (get_mesh(name)) {
			return KError::name_already_exists(name);
		}
		get_mesh(id)->set_name(name);
		return {};
	}

	ResourceManager::mesh_iterator ResourceManager::mesh_begin() {
		return mesh_iterator(_meshes.begin(), _meshes.end(), util::exists<Mesh>);
	}

	ResourceManager::mesh_iterator ResourceManager::mesh_end() {
		return mesh_iterator(_meshes.end(), _meshes.end(), util::exists<Mesh>);
	}
	ResourceManager::const_mesh_iterator ResourceManager::mesh_begin() const {
		return const_mesh_iterator(_meshes.begin(), _meshes.end(), util::exists<Mesh>);
	}
	ResourceManager::const_mesh_iterator ResourceManager::mesh_end() const {
		return const_mesh_iterator(_meshes.end(), _meshes.end(), util::exists<Mesh>);
	}
	ResourceManager::MeshContainer const &ResourceManager::mesh_container() const {
		return _meshes;
	}
	

	util::Result<void, KError> ResourceManager::add_mesh_observer(util::Observer *observer) {
		if (util::contains(_mesh_observers, observer)) {
			return KError::internal("Mesh observer already exists");
		}
		_mesh_observers.push_back(observer);
		for (auto &mesh : meshes()) {
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
		for (auto &m : materials()) {
			if (m->name() == name) {
				return m.get();
			}
		}
		return nullptr;
	}

	types::Material *ResourceManager::get_material(std::string const &name) {
		for (auto &m : materials()) {
			if (m->name() == name) {
				return m.get();
			}
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
		for (auto &m : materials()) {
			if (m->name() == name) return true; }
		return false;
	}

	util::Result<void, KError> ResourceManager::rename_material(uint32_t id, const std::string &name) {
		if (get_material(name)) {
			return KError::name_already_exists(name);
		}
		get_material(id)->set_name(name);
		return {};
	}

	ResourceManager::material_iterator ResourceManager::material_begin() {
		return material_iterator(_materials.begin(), _materials.end(), util::exists<Material>);
	}

	ResourceManager::material_iterator ResourceManager::material_end() {
		return material_iterator(_materials.end(), _materials.end(), util::exists<Material>);
	}
	ResourceManager::const_material_iterator ResourceManager::material_begin() const {
		return const_material_iterator(_materials.begin(), _materials.end(), util::exists<Material>);
	}
	ResourceManager::const_material_iterator ResourceManager::material_end() const {
		return const_material_iterator(_materials.end(), _materials.end(), util::exists<Material>);
	}
	ResourceManager::MaterialContainer const &ResourceManager::material_container() const {
		return _materials;
	}

	util::Result<void, KError> ResourceManager::add_material_observer(util::Observer *observer) {
		if (util::contains(_material_observers, observer)) {
			return KError::internal("Material observer already exists");
		}
		_material_observers.push_back(observer);
		for (auto &material : materials()) {
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
			std::unique_ptr<Mesh> &&mesh)
	{
		if (has_mesh(mesh->name())) {
			return KError::mesh_already_exists(mesh->name());
		}
		auto id = mesh->id();
		_meshes.push_back(std::move(mesh));
		for (auto &mesh_observer: _mesh_observers) {
			mesh_observer->obs_create(id);
		}
		return {id};
	}

	util::Result<uint32_t, KError> ResourceManager::_add_material(
			const std::string &name,
			std::unique_ptr<types::Material> &&material)
	{
		if (has_material(name)) {
			return KError::material_already_exists(name);
		}
		auto id = material->id();
		material->set_name(name);
		_materials.push_back(std::move(material));
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
