#include <vulkan/vulkan_core.h>

#include "Scene.hpp"
#include "PreviewRenderPass.hpp"

namespace vulkan {
	Scene::Scene(PreviewRenderPass::Ptr preview_render_pass):
		_preview_render_pass(std::move(preview_render_pass))
	{ }

	util::Result<Scene, KError> Scene::create(
			types::ResourceManager &resource_manager)
	{
		auto raytrace_render_pass = RaytraceRenderPass::create({300, 300});
		TRY(raytrace_render_pass);

		auto render_pass_res = PreviewRenderPass::create(
				resource_manager,
				{300, 300});
		if (!render_pass_res) {
			return render_pass_res.error();
		}
		auto scene = Scene(std::move(render_pass_res.value()));
		scene._raytrace_render_pass = std::move(raytrace_render_pass.value());
		return scene;
	}

	VkExtent2D Scene::size() const {
		return _preview_render_pass->size();
	}

	void Scene::resize(VkExtent2D new_size) {
		_preview_render_pass->resize(new_size);
	}

	void Scene::render_preview() {
		_preview_render_pass->submit([this](VkCommandBuffer command_buffer){
				for (auto &node : _nodes) {
					node.render_preview(*_preview_render_pass, command_buffer);
				}
		});
	}

	void Scene::render_raytrace() {
		_raytrace_render_pass->submit();
	}

	Texture& Scene::preview_texture() {
		return *_preview_render_pass;
	}
	Texture& Scene::raytrace_texture() {
		return *_raytrace_render_pass;
	}

	util::Result<void, KError> Scene::add_node(Node node) {
		if (!node.material().preview_impl()) {
			node.material().add_preview(*_preview_render_pass);
		}
		_nodes.push_back(std::move(node));
		return {};
	}
}
