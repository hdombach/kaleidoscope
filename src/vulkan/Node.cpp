#include <glm/fwd.hpp>
#include <vulkan/vulkan_core.h>

#include "Node.hpp"
#include "Material.hpp"
#include "PreviewRenderPass.hpp"

namespace vulkan {
	Node::Node(Node&& other):
		_mesh(other._mesh),
		_material(std::move(other._material)),
		_position(other._position)
	{ }

	void Node::render_preview(
			PreviewRenderPass &preview_render_pass,
			VkCommandBuffer command_buffer)
	{
		auto size = glm::vec2{
			preview_render_pass.size().width,
			preview_render_pass.size().height};
		_material->preview_impl()->update_uniform(
				preview_render_pass.frame_index(),
				_position,
				size);
		vkCmdBindPipeline(
				command_buffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				_material->preview_impl()->pipeline());

		VkBuffer vertex_buffers[] = {_mesh.vertexBuffer()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, _mesh.indexBuffer(), 0, VK_INDEX_TYPE_UINT32);

		auto descriptor_set = _material->preview_impl()->get_descriptor_set(preview_render_pass.frame_index());

		vkCmdBindDescriptorSets(
				command_buffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				_material->preview_impl()->pipeline_layout(),
				0,
				1,
				&descriptor_set,
				0,
				nullptr);

		vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(_mesh.indexCount()), 1, 0, 0, 0);
	}
}
