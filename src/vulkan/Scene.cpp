#include <algorithm>

#include <vulkan/vulkan_core.h>

#include "util/Util.hpp"
#include "Scene.hpp"
#include "prev_pass/PrevPass.hpp"
#include "Uniforms.hpp"
#include "util/map_iterator.hpp"

namespace vulkan {
	using util::f;

	util::Result<Scene::Ptr, Error> Scene::create(
			types::ResourceManager &resource_manager)
	{
		auto scene = Scene::Ptr(new Scene());
		scene->_resource_manager = &resource_manager; // Needs to be initialized before prev pass

		if (auto err = RayPass::create(*scene, {50, 500}).move_or(scene->_raytrace_render_pass)) {
			return Error(ErrorType::MISC, "Could not create raytrace render pass", err.value());
		}
		if (auto err = PrevPass::create(*scene, {300, 300}).move_or(scene->_preview_render_pass)) {
			return Error(ErrorType::MISC, "Could not create preview render pass", err.value());
		}
		if (auto err = InstancedPass::create({300, 300}, *scene).move_or(scene->_instanced_pass)) {
			return Error(ErrorType::MISC, "Could not create instanced pass", err.value());
		}

		scene->_is_preview = true;
		if (auto err = scene->add_node_observer(&scene->_preview_render_pass->node_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add preview render pass node observer", err.value());
		}
		if (auto err = resource_manager.add_mesh_observer(&scene->_preview_render_pass->mesh_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add preview render pass mesh observer", err.value());
		}
		if (auto err = resource_manager.add_material_observer(&scene->_preview_render_pass->material_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add preview render pass material observer", err.value());
		}

		if (auto err = scene->add_node_observer(&scene->_raytrace_render_pass->node_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add raytrace pass node observer", err.value());
		}
		if (auto err = resource_manager.add_mesh_observer(&scene->_raytrace_render_pass->mesh_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add raytrace pass mesh observer", err.value());
		}
		if (auto err = resource_manager.add_material_observer(&scene->_raytrace_render_pass->material_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add raytrace pass material observer", err.value());
		}

		if (auto err = scene->add_node_observer(&scene->_instanced_pass->node_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add instanced pass node observer", err.value());
		}
		if (auto err = resource_manager.add_mesh_observer(&scene->_instanced_pass->mesh_observer()).move_or()) {
			return Error(ErrorType::MISC, "Could not add instanced pass mesh observer", err.value());
		}

		scene->_root = scene->create_virtual_node().value();
		scene->_root->name() = "root";
		scene->_viewport_camera = types::Camera::create(scene->_nodes.get_id());
		scene->_viewport_camera->name() = "Viewport camera";

		return scene;
	}

	VkDescriptorSet Scene::imgui_descriptor_set() {
		if (_is_preview) {
			return _instanced_pass->imgui_descriptor_set();
			//return _preview_render_pass->imgui_descriptor_set();
		} else {
			return _raytrace_render_pass->imgui_descriptor_set();
		}
	}

	VkImageView Scene::image_view() {
		if (_is_preview) {
			return _instanced_pass->image_view();
			//return _preview_render_pass->image_view();
		} else {
			return _raytrace_render_pass->image_view();
		}
	}

	VkExtent2D Scene::size() const {
		return {300, 300};
		//return _preview_render_pass->size();
	}

	void Scene::resize(VkExtent2D new_size) {
		//_preview_render_pass->resize(new_size);
		_raytrace_render_pass->resize(new_size);
	}

	int Scene::render_rate() const {
		return _render_rate;
	}

	void Scene::set_render_rate(int rate) {
		if (rate < 200) {
			rate = 200;
		}
		_render_rate = rate;
	}

	void Scene::set_is_preview(bool is_preview) {
		if (_is_preview && !is_preview) {
			_raytrace_render_pass->reset_counters();
		}
		_is_preview = is_preview;
	}

	VkSemaphore Scene::render_preview(VkSemaphore semaphore) {
		return _instanced_pass->render(semaphore, camera());
		//return _preview_render_pass->render(_nodes.raw(), camera(), semaphore);
	}

	VkSemaphore Scene::render_raytrace(VkSemaphore semaphore) {
		auto uniform_buffer = ComputeUniform{};
		uniform_buffer.camera_rotation = camera().rotation_matrix();
		uniform_buffer.camera_translation = camera().get_matrix() * glm::vec4(0, 0, 0, 1.0);
		uniform_buffer.aspect = static_cast<float>(camera().width()) / static_cast<float>(camera().height());
		uniform_buffer.fovy = camera().fovy();
		uniform_buffer.de_iterations = camera().de_iterations();
		uniform_buffer.de_small_step = std::pow(10.0f, -camera().de_small_step());

		return _raytrace_render_pass->submit(*_nodes.raw()[0], _render_rate, uniform_buffer, semaphore);
	}

	void Scene::update() {
		for (auto &node : *this) {
			if (node->dirty_bits()) {
				update_node(node->id());
				node->clear_dirty_bits();
			}
		}
		if (_camera_dirty_bit) {
			VkExtent2D new_size;
			new_size.width = camera().width();
			new_size.height = camera().height();
			resize(new_size);
		}
	}

	void Scene::set_active_camera(uint32_t id) {
		_camera_dirty_bit = true;
		_active_camera = id;
	}

	uint32_t Scene::camera_id() {
		return _active_camera;
	}

	types::Camera &Scene::camera() {
		if (_active_camera == 0) {
			return *_viewport_camera;
		} else {
			return *static_cast<types::Camera*>(_nodes[_active_camera].get());
		}
	}

	Node const *Scene::get_node(uint32_t id) const {
		if (id >= 0 && id < _nodes.size()) {
			return _nodes[id].get();
		} else {
			return nullptr;
		}
	}

	Node *Scene::get_node_mut(uint32_t id) {
		if (id >= 0 && id < _nodes.size()) {
			if (_nodes[id] && _nodes[id]->type() == vulkan::Node::Type::Camera) {
				_camera_dirty_bit = true;
			}
			return _nodes[id].get();
		} else {
			return nullptr;
		}
	}

	util::Result<Node *, Error> Scene::create_node(
			types::Mesh const *mesh,
			types::Material const *material)
	{
		if (!mesh) {
			return Error(ErrorType::INVALID_ARG, "Mesh is null");
		}
		if (!material) {
			return Error(ErrorType::INVALID_ARG, "Material is null");
		}
		auto id = _nodes.get_id();
		_nodes.insert(Node::create(id, *mesh, *material));
		for (auto &observer : _node_observers) {
			observer->obs_create(id);
		}
		auto node = _nodes[id].get();
		node->move_to(_root);
		return node;
	}

	util::Result<Node *, Error> Scene::create_virtual_node() {
		auto id = _nodes.get_id();
		_nodes.insert(Node::create_virtual(id));
		for (auto &observer : _node_observers) {
			observer->obs_create(id);
		}
		auto node = _nodes[id].get();
		node->move_to(_root);
		return node;
	}

	util::Result<types::Camera *, Error> Scene::create_camera() {
		auto id = _nodes.get_id();
		_nodes.insert(types::Camera::create(id));
		for (auto &observer : _node_observers) {
			observer->obs_create(id);
		}
		auto node = _nodes[id].get();
		node->move_to(_root);
		return static_cast<types::Camera *>(node);
	}

	util::Result<void, Error> Scene::remove_node(uint32_t id) {
		if (!_nodes.contains(id)) {
			return Error(ErrorType::INVALID_ARG, f("Invalid node id ", id));
		}

		if (id == _root->id()) {
			return Error(ErrorType::INVALID_ARG, "Cannot remove root node");
		}

		for (auto child : *_nodes[id].get()) {
			if (auto err = remove_node(child->id()).move_or()) {
				return Error(ErrorType::MISC, f("Cannot remove node ", id), err.value());
			}
		}

		for (auto &observer : _node_observers) {
			observer->obs_remove(id);
		}

		_nodes[id]->parent()->remove_child(id);
		_nodes[id].reset();
		return {};
	}

	types::ResourceManager &Scene::resource_manager() {
		return *_resource_manager;
	}

	Node *Scene::root() {
		return _root;
	}

	Node const *Scene::root() const {
		return _root;
	}

	util::Result<void, Error> Scene::add_node_observer(util::Observer *observer) {
		if (util::contains(_node_observers, observer)) {
			return Error(ErrorType::INVALID_ARG, "Node observer already exists");
		}
		_node_observers.push_back(observer);
		for (auto &node : _nodes) {
			observer->obs_create(node->id());
		}
		return {};
	}

	util::Result<void, Error> Scene::rem_node_observer(util::Observer *observer) {
		if (util::contains(_node_observers, observer)) {
			return Error(ErrorType::INVALID_ARG, "Node observer does not exist");
		}
		std::ignore = std::remove(
				_node_observers.begin(),
				_node_observers.end(),
				observer);
		return {};
	}

	Scene::Container const &Scene::nodes() const {
		return _nodes;
	}

	Scene::Container &Scene::nodes() {
		return _nodes;
	}

	Scene::iterator Scene::begin() {
		return _nodes.begin();
	}

	Scene::iterator Scene::end() {
		return _nodes.end();
	}

	Scene::const_iterator Scene::begin() const {
		return _nodes.begin();
	}

	Scene::const_iterator Scene::end() const {
		return _nodes.end();
	}

	inline types::Camera *_cam_cast(Scene::iterator node) {
		if ((*node)->type() != Node::Type::Camera) return nullptr;
		return static_cast<types::Camera *>((*node).get());
	}

	Scene::camera_iterator Scene::cameras_begin() {
		auto test = _nodes.begin();
		using M = util::map_iterator<Scene::iterator, types::Camera *>;
		return camera_iterator(
			M(_nodes.begin(), _cam_cast),
			M(_nodes.end(), _cam_cast)
		);
	}

	Scene::camera_iterator Scene::cameras_end() {
		using M = util::map_iterator<Scene::iterator, types::Camera *>;
		return camera_iterator(
			M(_nodes.end(), _cam_cast),
			M(_nodes.end(), _cam_cast)
		);
	}
}
