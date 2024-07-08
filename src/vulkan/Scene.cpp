#include <algorithm>

#include <vulkan/vulkan_core.h>

#include "../util/Util.hpp"
#include "Scene.hpp"
#include "PrevPass.hpp"
#include "UniformBufferObject.hpp"

namespace vulkan {
	util::Result<Scene::Ptr, KError> Scene::create(
			types::ResourceManager &resource_manager)
	{
		auto scene = Scene::Ptr(new Scene());

		auto raytrace_render_pass = RaytraceRenderPass::create({300, 300});
		TRY(raytrace_render_pass);

		auto render_pass_res = PrevPass::create(
				*scene,
				{300, 300});
		if (!render_pass_res) {
			return render_pass_res.error();
		}
		scene->_resource_manager = &resource_manager;
		scene->_preview_render_pass = std::move(render_pass_res.value());
		scene->_raytrace_render_pass = std::move(raytrace_render_pass.value());
		scene->_is_preview = true;
		scene->add_node_observer(&scene->_preview_render_pass->node_observer());
		resource_manager.add_mesh_observer(&scene->_preview_render_pass->mesh_observer());
		resource_manager.add_material_observer(&scene->_preview_render_pass->material_observer());
		return scene;
	}

	VkDescriptorSet Scene::get_descriptor_set() {
		return _cur_texture().get_descriptor_set();
	}

	ImageView const &Scene::image_view() {
		return _cur_texture().image_view();
	}

	VkExtent2D Scene::size() const {
		return _preview_render_pass->size();
	}

	void Scene::resize(VkExtent2D new_size) {
		_preview_render_pass->resize(new_size);
		_camera.width = new_size.width;
		_camera.height = new_size.height;
	}

	void Scene::set_is_preview(bool is_preview) {
		_is_preview = is_preview;
	}

	void Scene::render_preview() {
		_preview_render_pass->render(_nodes, camera());
	}

	void Scene::render_raytrace() {
		auto uniform_buffer = ComputeUniformBuffer{};
		uniform_buffer.camera_rotation = camera().gen_rotate_mat();
		uniform_buffer.camera_translation = glm::vec4(camera().position, 0.0);
		_raytrace_render_pass->current_uniform_buffer().set_value(uniform_buffer);
		_raytrace_render_pass->submit(_nodes[0]);
	}

	Node const *Scene::get_node(uint32_t id) const {
		return &_nodes[id];
	}

	Node *Scene::get_node_mut(uint32_t id) {
		return &_nodes[id];
	}

	util::Result<uint32_t, KError> Scene::add_node(
			types::Mesh const *mesh,
			types::Material const *material)
	{
		if (!mesh) {
			return KError::invalid_arg("Mesh is null");
		}
		if (!material) {
			return KError::invalid_arg("Material is null");
		}
		auto id = _get_node_id();
		_nodes.push_back(Node(id, *mesh, *material));
		for (auto &observer : _node_observers) {
			observer->obs_create(id);
		}
		return id;
	}

	types::ResourceManager &Scene::resource_manager() {
		return *_resource_manager;
	}

	util::Result<void, KError> Scene::add_node_observer(util::Observer *observer) {
		if (util::contains(_node_observers, observer)) {
			return KError::internal("Node observer already exists");
		}
		_node_observers.push_back(observer);
		for (auto &node : _nodes) {
			observer->obs_create(node.id());
		}
		return {};
	}

	util::Result<void, KError> Scene::rem_node_observer(util::Observer *observer) {
		if (util::contains(_node_observers, observer)) {
			return KError::internal("Node observer does not exist");
		}
		std::ignore = std::remove(
				_node_observers.begin(),
				_node_observers.end(),
				observer);
		return {};
	}

	Texture& Scene::_cur_texture() {
		if (_is_preview) {
			return *_preview_render_pass;
		} else {
			return *_raytrace_render_pass;
		}
	}

	uint32_t Scene::_get_node_id() {
		return _nodes.size();
	}
}
