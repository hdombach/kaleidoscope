
#include <algorithm>
#include <string>
#include <vector>

#include "util/result.hpp"
#include "util/file.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/Observer.hpp"
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
		//Can't use defaultTexture_ for holder because of const issues
		auto res = add_texture_from_file("assets/default.png");
		if (res) {
			_default_texture = res.value();
		} else {
			log_fatal_error(res.error()) << std::endl;
		}

		if (auto id = add_color_material("Default Material", glm::vec3(1.0, 0.0, 1.0))) {
			_default_material = id.value();
		} else {
			log_fatal_error() << "Couldn't create default material" << std::endl;
		}

		if (auto id = add_mesh_square("Default mesh")) {
			_default_mesh = id.value();
		} else {
			log_fatal_error() << "Couldn't create default mesh" << std::endl;
		}
	}

	ResourceManager::~ResourceManager() {
		_textures.clear();
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_texture_from_file(
			const std::string &url)
	{
		auto id = _textures.get_id();
		auto path = std::string();
		auto texture = vulkan::StaticTexture::Ptr();

		if (auto err = util::env_file_path(url).move_or(path)) {
			return Error(ErrorType::MISC, "Could not open texture", err.value());
		}
		if (auto err = vulkan::StaticTexture::from_file(id, path).move_or(texture)) {
			return Error(ErrorType::VULKAN, "Could not create texture", err.value());
		}

		int i = 0;
		auto base_name = texture->name();
		while (get_texture(texture->name())) {
			texture->set_name(base_name + "_" + std::to_string(i));
			i++;
		}

		assert(_textures.insert(std::move(texture)));

		return  {id};
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

	util::Result<void, ResourceManager::Error> ResourceManager::rename_texture(uint32_t id, const std::string &name) {
		if (get_texture(name)) {
			return Error(ErrorType::DUPLICATE_ENTRY, util::f("Duplicate texture name ", name));
		}
		get_texture(id)->set_name(name);
		return {};
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_mesh_from_file(
			std::string const &url)
	{
		auto id = _meshes.get_id();
		auto path = std::string();
		auto mesh = StaticMesh::Ptr();

		if (auto err = util::env_file_path(url).move_or(path)) {
			return Error(ErrorType::MISC, "Could not get env file path", err.value());
		}

		if (auto err = StaticMesh::from_file(id, path).move_or(mesh)) {
			return Error(ErrorType::MISC, "Could not load mesh", err.value());
		}

		int i = 1;
		auto base_name = mesh->name();
		while (get_mesh(mesh->name())) {
			mesh->set_name(base_name + "_" + std::to_string(i));
			i++;
		}

		return _add_mesh(std::move(mesh));
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_mesh_square(
			std::string const &name)
	{
		return _add_mesh(StaticMesh::create_square(name, _meshes.get_id()));
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_mesh_from_vertices(
			std::string const &name,
			std::vector<vulkan::Vertex> const &vertices)
	{
		return _add_mesh(StaticMesh::from_vertices(
			name,
			_meshes.get_id(),
			vertices
		));
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_mesh_mandelbulb(
			std::string const &name) 
	{
		return _add_mesh(MandelbulbMesh::create(name, _meshes.get_id()));
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_mesh_mandelbox(
		std::string const &name)
		{
			return _add_mesh(MandelboxMesh::create(name, _meshes.get_id()));
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

	util::Result<void, ResourceManager::Error> ResourceManager::rename_mesh(
			uint32_t id,
			const std::string &name)
	{
		if (get_mesh(name)) {
			return Error(ErrorType::DUPLICATE_ENTRY, util::f("Mesh ", name, " already exists"));
		}
		get_mesh(id)->set_name(name);
		return {};
	}

	util::Result<void, ResourceManager::Error> ResourceManager::add_mesh_observer(util::Observer *observer) {
		if (util::contains(_mesh_observers, observer)) {
			return Error(ErrorType::DUPLICATE_ENTRY, "Mesh observer already exists");
		}
		_mesh_observers.push_back(observer);
		for (auto &mesh : meshes()) {
			observer->obs_create(mesh->id());
		}
		return {};
	}

	util::Result<void, ResourceManager::Error> ResourceManager::rem_mesh_observer(util::Observer *observer) {
		if (util::contains(_mesh_observers, observer)) {
			return Error(ErrorType::MISSING_ENTRY, "Mesh observer does not exist");
		}
		std::ignore = std::remove(
				_mesh_observers.begin(),
				_mesh_observers.end(),
				observer);
		return {};
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_texture_material(
			std::string const &name,
			vulkan::Texture *texture)
	{
		return _add_material(name, types::TextureMaterial::create(_materials.get_id(), texture));
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_color_material(
			std::string const &name,
			glm::vec3 color)
	{
		return _add_material(name, types::ColorMaterial::create(_materials.get_id(), color));
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::add_comb_texture_material(
			std::string const &name,
			vulkan::Texture *prim_texture,
			vulkan::Texture *comb_texture)
	{
		return _add_material(name, types::CombTextureMaterial::create(
			_materials.get_id(),
			prim_texture,
			comb_texture
		));
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

	util::Result<void, ResourceManager::Error> ResourceManager::rename_material(uint32_t id, const std::string &name) {
		if (get_material(name)) {
			return Error(ErrorType::DUPLICATE_ENTRY, util::f("Mesh with name ", name, " already exists"));
		}
		get_material(id)->set_name(name);
		return {};
	}

	util::Result<void, ResourceManager::Error> ResourceManager::add_material_observer(util::Observer *observer) {
		if (util::contains(_material_observers, observer)) {
			return Error(ErrorType::DUPLICATE_ENTRY, "Material observer already exists");
		}
		_material_observers.push_back(observer);
		for (auto &material : materials()) {
			observer->obs_create(material->id());
		}
		return {};
	}

	util::Result<void, ResourceManager::Error> ResourceManager::rem_material_observer(util::Observer *observer) {
		if (util::contains(_material_observers, observer)) {
			return Error(ErrorType::MISSING_ENTRY, "Material observer does not exist");
		}
		std::ignore = std::remove(
				_material_observers.begin(),
				_material_observers.end(),
				observer);
		return {};
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::_add_mesh(
			std::unique_ptr<Mesh> &&mesh)
	{
		if (has_mesh(mesh->name())) {
			return Error(ErrorType::DUPLICATE_ENTRY, util::f("Mesh with name ", mesh->name(), " already exists"));
		}
		auto id = mesh->id();
		_meshes.insert(std::move(mesh));

		for (auto &mesh_observer: _mesh_observers) {
			mesh_observer->obs_create(id);
		}
		return {id};
	}

	util::Result<uint32_t, ResourceManager::Error> ResourceManager::_add_material(
			const std::string &name,
			std::unique_ptr<types::Material> &&material)
	{
		if (has_material(name)) {
			return Error(ErrorType::DUPLICATE_ENTRY, util::f("Material with name ", material->name(), " already exists"));
		}
		auto id = material->id();
		material->set_name(name);
		_materials.insert(std::move(material));

		for (auto &material_observer: _material_observers) {
			material_observer->obs_create(id);
		}
		return {id};
	}
}

template<>
const char *types::ResourceManager::Error::type_str(types::ResourceManager::ErrorType t) {
	switch (t) {
		case types::ResourceManager::ErrorType::DUPLICATE_ENTRY:
			return "ResourceManager.DUPLICATE_ENTRY";
		case types::ResourceManager::ErrorType::MISSING_ENTRY:
			return "ResourceManager.MISSING_ENTRY";
		case types::ResourceManager::ErrorType::MISC:
			return "ResourceManager.MISC";
		case types::ResourceManager::ErrorType::VULKAN:
			return "ResourceManager.VULKAN";
	}
}

std::ostream &operator<<(std::ostream &os, types::ResourceManager::ErrorType t) {
	return os << types::ResourceManager::Error::type_str(t);
}
