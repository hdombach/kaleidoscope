#include <array>
#include <vector>
#include <functional>

#include <vulkan/vulkan_core.h>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "PreviewRenderPass.hpp"
#include "Scene.hpp"
#include "defs.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "../util/log.hpp"
#include "imgui_impl_vulkan.h"

namespace vulkan {
	/************************ PreviewRenderPass *********************************/

	util::Result<PreviewRenderPass::Ptr, KError> PreviewRenderPass::create(
			Scene &scene,
			VkExtent2D size)
	{
		auto result = std::unique_ptr<PreviewRenderPass>(
				new PreviewRenderPass(scene, size));
		TRY(result->_create_sync_objects());
		result->_create_command_buffers();
		result->_descriptor_pool = DescriptorPool::create();

		/* Create render pass */
		auto color_attachment = VkAttachmentDescription{};
		color_attachment.format = _RESULT_IMAGE_FORMAT;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto color_attachment_ref = VkAttachmentReference{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto depth_attachment = VkAttachmentDescription{};
		depth_attachment.format = _depth_format();
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		auto depth_attachment_ref = VkAttachmentReference{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		auto attachments = std::array<VkAttachmentDescription, 2>{
			color_attachment,
			depth_attachment,
		};

		auto render_pass_info = VkRenderPassCreateInfo{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;

		auto dependency = VkSubpassDependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		auto res = vkCreateRenderPass(
				Graphics::DEFAULT->device(),
				&render_pass_info,
				nullptr,
				&result->_render_pass);

		if (res != VK_SUCCESS) {
			return {res};
		}

		TRY(result->_create_images());

		/* create descriptor sets */
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
			auto buffer_res = MappedGlobalUniform::create();
			TRY(buffer_res);
			result->_mapped_uniforms.push_back(std::move(buffer_res.value()));
		}

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();
		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					0, 
					VK_SHADER_STAGE_VERTEX_BIT, 
					result->_mapped_uniforms));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates, 
				FRAMES_IN_FLIGHT, 
				result->_descriptor_pool);
		TRY(descriptor_sets);
		result->_descriptor_sets = std::move(descriptor_sets.value());

		return result;
	}

	PreviewRenderPass::PreviewRenderPass(
			Scene &scene, VkExtent2D size):
		_scene(&scene),
		_size(size)
	{
	}

	PreviewRenderPass::~PreviewRenderPass() {
		LOG_MEMORY << "Deconstructing main render pipeline" << std::endl;

		_cleanup_images();
		_descriptor_sets.clear();
		if (_render_pass) {
			vkDestroyRenderPass(Graphics::DEFAULT->device(), _render_pass, nullptr);
			_render_pass = nullptr;
		}

		_mapped_uniforms.clear();
		
		_in_flight_fences.clear();
		_render_finished_semaphores.clear();
	}

	void PreviewRenderPass::render(std::vector<Node> &nodes, types::Camera &camera) {
		require(_in_flight_fences[_frame_index].wait());
		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		_in_flight_fences[_frame_index].reset();

		auto command_buffer = _command_buffers[_frame_index];

		vkResetCommandBuffer(command_buffer, 0);

		auto begin_info = VkCommandBufferBeginInfo{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;

		require(vkBeginCommandBuffer(command_buffer, &begin_info));

		auto render_pass_info = VkRenderPassBeginInfo{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = _render_pass;
		render_pass_info.framebuffer = _framebuffers[_frame_index];
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = _size;

		auto clear_values = std::array<VkClearValue, 2>{};
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[1].depthStencil = {1.0f, 0};

		render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(_size.width);
		viewport.height = static_cast<float>(_size.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = _size;
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		auto uniform_buffer = GlobalUniformBuffer{};
		uniform_buffer.camera_transformation = camera.gen_raster_mat();
		current_uniform_buffer().set_value(uniform_buffer);


		for (auto &node : nodes) {
			auto size = glm::vec2{_size.width, _size.height};
			node.material().preview_impl()->update_uniform(
					_frame_index, 
					node.position(), 
					size);
			vkCmdBindPipeline(
					command_buffer, 
					VK_PIPELINE_BIND_POINT_GRAPHICS, 
					node.material().preview_impl()->pipeline());

			auto &mesh = _meshes[node.mesh().id()];

			VkBuffer vertex_buffers[] = {mesh.vertex_buffer()};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

			auto descriptor_sets = std::array<VkDescriptorSet, 2>{
				global_descriptor_set(_frame_index),	
				node.material().preview_impl()->get_descriptor_set(_frame_index),
			};

			vkCmdBindDescriptorSets(
					command_buffer, 
					VK_PIPELINE_BIND_POINT_GRAPHICS, 
					node.material().preview_impl()->pipeline_layout(),
					0,
					descriptor_sets.size(),
					descriptor_sets.data(),
					0,
					nullptr);

			vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.index_count()), 1, 0, 0, 0);

		}

		vkCmdEndRenderPass(command_buffer);
		require(vkEndCommandBuffer(command_buffer));


		auto wait_stages = std::array<VkPipelineStageFlags, 2>{VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submit_info.pWaitDstStageMask = wait_stages.data();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &_command_buffers[_frame_index];
		//submitInfo.signalSemaphoreCount = 1;
		//submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[_frame_index];

		require(vkQueueSubmit(Graphics::DEFAULT->graphics_queue(), 1, &submit_info, *_in_flight_fences[_frame_index]));

		_frame_index = (_frame_index + 1) % FRAMES_IN_FLIGHT;

	}

	void PreviewRenderPass::resize(VkExtent2D size) {
		if (size.width == _size.width && size.height == _size.height) return;
		Graphics::DEFAULT->wait_idle();
		_cleanup_images();
		_size = size;
		auto res = _create_images();
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}
		//submit(); TODO

		/*
		 * When resize, the entire queue in the frame buffer is cleared. This causes
		 * flickering when resizing the window since there is nothing to display.
		 * My temporary solution is to move back the frame index so that it is more
		 * immediately shown. Still cuases slight flickering though.
		 * True solution is to resize the images used not delete and create again.
		 */
		_frame_index--;
		if (_frame_index < 0) {
			_frame_index = FRAMES_IN_FLIGHT - 1;
		}
	}
	bool PreviewRenderPass::is_resizable() const {
		return true;
	}

	VkExtent2D PreviewRenderPass::size() const {
		return _size;
	}

	VkDescriptorSet PreviewRenderPass::get_descriptor_set() {
		return _imgui_descriptor_sets[_frame_index];
	}

	ImageView const &PreviewRenderPass::image_view() {
		return _color_image_views[_frame_index];
	}
	VkRenderPass PreviewRenderPass::render_pass() {
		return _render_pass;
	}
	MappedGlobalUniform &PreviewRenderPass::current_uniform_buffer() {
		return _mapped_uniforms[_frame_index];
	}

	void PreviewRenderPass::obs_create(uint32_t id) {
		LOG_DEBUG << "Creating preview mesh" << std::endl;
		while (id + 1 > _meshes.size()) {
			_meshes.push_back(PreviewRenderPassMesh());
		}
		if (auto mesh = PreviewRenderPassMesh::create(*_scene, _scene->resource_manager().get_mesh(id))) {
			_meshes[id] = std::move(mesh.value());
		} else {
			LOG_ERROR << "Couldn't create preview mesh: " << mesh.error().desc() << std::endl;
		}
	}

	void PreviewRenderPass::obs_update(uint32_t id) { }

	void PreviewRenderPass::obs_remove(uint32_t id) {
		_meshes[id].destroy();
	}

	util::Result<void, KError> PreviewRenderPass::_create_sync_objects() {
		_render_finished_semaphores.resize(FRAMES_IN_FLIGHT);
		_in_flight_fences.resize(FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto semaphore = Semaphore::create();
			TRY(semaphore);
			_render_finished_semaphores[i] = std::move(semaphore.value());

			auto fence = Fence::create();
			TRY(fence);
			_in_flight_fences[i] = std::move(fence.value());
		}
		return {};
	}

	void PreviewRenderPass::_create_command_buffers() {
		_command_buffers.resize(FRAMES_IN_FLIGHT);
		auto alloc_info = VkCommandBufferAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = Graphics::DEFAULT->command_pool();
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = (uint32_t) _command_buffers.size();

		require(vkAllocateCommandBuffers(Graphics::DEFAULT->device(), &alloc_info, _command_buffers.data()));
	}

	util::Result<void, KError> PreviewRenderPass::_create_images() {
		_cleanup_images();
		/* create depth resources */
		{
			auto image_res = Image::create(
					_size.width,
					_size.height,
					_depth_format(),
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			TRY(image_res);
			_depth_image = std::move(image_res.value());

			auto image_view_res = _depth_image.create_image_view_full(
					_depth_format(), 
					VK_IMAGE_ASPECT_DEPTH_BIT, 
					1);
			TRY(image_view_res);
			_depth_image_view = std::move(image_view_res.value());

			Graphics::DEFAULT->transition_image_layout(
					_depth_image.value(),
					_depth_format(),
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					1);
		}

		if (_color_images.size() != 0) {
			return KError::internal("_color_images in PreviewRenderPass must be of size 0");
		}
		if (_color_image_views.size()  != 0) {
			return KError::internal("_color_image_views in PreviewRenderPass must be of size 0");
		}
		if (_framebuffers.size() != 0) {
			return KError::internal("_framebuffers in PreviewRenderPass must be of size 0");
		}
		if (_imgui_descriptor_sets.size() != 0) {
			return KError::internal("_imgui_descriptor_sets in PreviewRenderPass must be of size 0");
		}
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto image_res = Image::create(
					_size.width,
					_size.height,
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
						| VK_IMAGE_USAGE_STORAGE_BIT
						| VK_IMAGE_USAGE_SAMPLED_BIT);
			TRY(image_res);
			_color_images.push_back(std::move(image_res.value()));

			auto image_view_res = _color_images[i].create_image_view_full(
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_ASPECT_COLOR_BIT,
					1);
			TRY(image_view_res);
			_color_image_views.push_back(std::move(image_view_res.value()));

			Graphics::DEFAULT->transition_image_layout(
					_color_images[i].value(),
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL,
					1);

			auto attachments = std::array<VkImageView, 2>{
				_color_image_views[i].value(),
				_depth_image_view.value(),
			};

			auto framebuffer_info = VkFramebufferCreateInfo{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = _render_pass;
			framebuffer_info.attachmentCount = attachments.size();
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = _size.width;
			framebuffer_info.height = _size.height;
			framebuffer_info.layers = 1;

			_framebuffers.push_back(nullptr);
			auto res = vkCreateFramebuffer(
					Graphics::DEFAULT->device(),
					&framebuffer_info,
					nullptr,
					&_framebuffers[i]);

			_imgui_descriptor_sets.push_back(ImGui_ImplVulkan_AddTexture(
					Graphics::DEFAULT->main_texture_sampler(),
					_color_image_views[i].value(),
					VK_IMAGE_LAYOUT_GENERAL));
		}


		return {};
	}

	void PreviewRenderPass::_cleanup_images() {
		_depth_image.destroy();
		_depth_image_view.destroy();
		_color_image_views.clear();
		_color_images.clear();

		for (auto framebuffer : _framebuffers) {
			vkDestroyFramebuffer(Graphics::DEFAULT->device(), framebuffer, nullptr);
		}
		_framebuffers.clear();

		for (auto descriptor_set : _imgui_descriptor_sets) {
			ImGui_ImplVulkan_RemoveTexture(descriptor_set);
		}
		_imgui_descriptor_sets.clear();
	}

	VkFormat PreviewRenderPass::_depth_format() {
		return Graphics::DEFAULT->find_supported_format(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}
