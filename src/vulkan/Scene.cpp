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
		auto render_pass_res = PreviewRenderPass::create(
				resource_manager,
				{300, 300});
		if (!render_pass_res) {
			return render_pass_res.error();
		} else {
			return Scene(std::move(render_pass_res.value()));
		}
	}

	VkExtent2D Scene::size() const {
		return _preview_render_pass->size();
	}

	void Scene::resize(VkExtent2D new_size) {
		_preview_render_pass->resize(new_size);
	}

	void Scene::render_preview() {
		return _preview_render_pass->submit([this](VkCommandBuffer command_buffer){
				for (auto node : _nodes) {
					vkCmdBindPipeline(
							command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							node.material().preview_impl()->pipeline());

					VkBuffer vertex_buffers[] = {node.mesh().vertexBuffer()};
					VkDeviceSize offsets[] = {0};
					vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
					vkCmdBindIndexBuffer(command_buffer, node.mesh().indexBuffer(), 0, VK_INDEX_TYPE_UINT32);

					auto descriptor_set = node.material().preview_impl()->get_descriptor_set(0); // TODO: use frame_index to get descriptor set

					vkCmdBindDescriptorSets(
							command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							node.material().preview_impl()->pipeline_layout(),
							0,
							1,
							&descriptor_set,
							0,
							nullptr);

					vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(node.mesh().indexCount()), 1, 0, 0, 0);
				}
		});
	}

	Texture& Scene::preview_texture() {
		return *_preview_render_pass;
	}

	util::Result<void, KError> Scene::add_node(Node node) {
		if (!node.material().preview_impl()) {
			node.material().add_preview(*_preview_render_pass);
		}
		_nodes.push_back(node);
		return {};
	}
}
