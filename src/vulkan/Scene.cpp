#include <vulkan/vulkan_core.h>

#include "Scene.hpp"
#include "PreviewRenderPass.hpp"
#include "UniformBufferObject.hpp"

namespace vulkan {
	util::Result<Scene::Ptr, KError> Scene::create(
			types::ResourceManager &resource_manager)
	{
		auto scene = Scene::Ptr(new Scene());

		auto raytrace_render_pass = RaytraceRenderPass::create({300, 300});
		TRY(raytrace_render_pass);

		auto render_pass_res = PreviewRenderPass::create(
				*scene,
				{300, 300});
		if (!render_pass_res) {
			return render_pass_res.error();
		}
		scene->_resource_manager = &resource_manager;
		scene->_preview_render_pass = std::move(render_pass_res.value());
		scene->_raytrace_render_pass = std::move(raytrace_render_pass.value());
		scene->_is_preview = true;
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

	util::Result<void, KError> Scene::add_node(Node node) {
		if (!node.material().preview_impl()) {
			node.material().add_preview(*_preview_render_pass);
		}
		_nodes.push_back(std::move(node));
		return {};
	}

	types::ResourceManager &Scene::resource_manager() {
		return *_resource_manager;
	}

	Texture& Scene::_cur_texture() {
		if (_is_preview) {
			return *_preview_render_pass;
		} else {
			return *_raytrace_render_pass;
		}
	}
}
