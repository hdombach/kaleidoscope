#include <cstdint>
#include <cstdio>
#include <cstring>
#include <array>
#include <memory>

#include "mainRenderPipeline.hpp"
#include "defs.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "imgui_impl_vulkan.h"
#include "../util/log.hpp"
#include "semaphore.hpp"
#include "uniformBufferObject.hpp"
#include "imageView.hpp"

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <glm/fwd.hpp>
#include <stb_image.h>

namespace vulkan {
	util::Result<MainRenderPipeline::Ptr, KError> MainRenderPipeline::create(
			types::ResourceManager &resource_manager,
			VkExtent2D size)
	{
		auto result = std::unique_ptr<MainRenderPipeline>(
				new MainRenderPipeline(resource_manager, size));
		TRY(result->_create_sync_objects());
		result->_create_command_buffers();

		auto render_pass_res = PreviewRenderPass::create(size);
		TRY(render_pass_res);
		result->_preview_render_pass = std::move(render_pass_res.value());

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
			auto buffer_res = MappedUniformObject::create();
			TRY(buffer_res);
			result->_mapped_uniforms.push_back(std::move(buffer_res.value()));
		}
		return result;
	}

	MainRenderPipeline::MainRenderPipeline(
			types::ResourceManager &resourceManager,
			VkExtent2D size):
		_size(size),
		_resource_manager(resourceManager)
	{
	}

	MainRenderPipeline::~MainRenderPipeline() {
		util::log_memory("Deconstructing main render pipeline");

		_mapped_uniforms.clear();
		
		_in_flight_fences.clear();
		_render_finished_semaphores.clear();
	}

	void MainRenderPipeline::submit() {
		require(_in_flight_fences[_frame_index].wait());
		if (_framebuffer_resized ) {
			_framebuffer_resized = false;
			_recreate_result_images();
		}
		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		_update_uniform_buffer(_frame_index);

		_in_flight_fences[_frame_index].reset();

		vkResetCommandBuffer(_command_buffers[_frame_index], 0);

		_record_command_buffer(_command_buffers[_frame_index]);

		auto wait_stages = std::array<VkPipelineStageFlags, 2>{VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submit_info.pWaitDstStageMask = wait_stages.data();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &_command_buffers[_frame_index];
		//submitInfo.signalSemaphoreCount = 1;
		//submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[_frame_index];

		require(vkQueueSubmit(Graphics::DEFAULT->graphicsQueue(), 1, &submit_info, *_in_flight_fences[_frame_index]));

		_frame_index = (_frame_index + 1) % FRAMES_IN_FLIGHT;
	}

	void MainRenderPipeline::resize(glm::ivec2 size) {
		_framebuffer_resized = true;
		this->_size = VkExtent2D{static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};
	}
	bool MainRenderPipeline::is_resizable() const {
		return true;
	}

	VkExtent2D MainRenderPipeline::get_size() const {
		return _size;
	}

	VkDescriptorSet MainRenderPipeline::get_descriptor_set() {
		return _preview_render_pass.imgui_descriptor_set(_frame_index);
	}

	ImageView const &MainRenderPipeline::image_view() {
		return _preview_render_pass.color_image_view(_frame_index);
	}
	VkRenderPass MainRenderPipeline::render_pass() {
		return _preview_render_pass.render_pass();
	}
	std::vector<MappedUniformObject> const &MainRenderPipeline::uniform_buffers() const {
		return _mapped_uniforms;
	}

	util::Result<void, KError> MainRenderPipeline::_create_sync_objects() {
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

	void MainRenderPipeline::_create_command_buffers() {
		_command_buffers.resize(FRAMES_IN_FLIGHT);
		auto alloc_info = VkCommandBufferAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = Graphics::DEFAULT->commandPool();
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = (uint32_t) _command_buffers.size();

		require(vkAllocateCommandBuffers(Graphics::DEFAULT->device(), &alloc_info, _command_buffers.data()));
	}


	void MainRenderPipeline::_recreate_result_images() {
		Graphics::DEFAULT->waitIdle();
		_preview_render_pass.resize(_size);
	}

	void MainRenderPipeline::_record_command_buffer(VkCommandBuffer commandBuffer) {
		auto main_mesh = _resource_manager.getMesh("viking_room");
		auto main_material = _resource_manager.getMaterial("viking_room");

		auto begin_info = VkCommandBufferBeginInfo{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;

		require(vkBeginCommandBuffer(commandBuffer, &begin_info));

		auto render_pass_info = VkRenderPassBeginInfo{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = _preview_render_pass.render_pass();
		render_pass_info.framebuffer = _preview_render_pass.framebuffer(_frame_index);
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = _size;

		auto clear_values = std::array<VkClearValue, 2>{};
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[1].depthStencil = {1.0f, 0};

		render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(commandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, main_material->pipeline());

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(_size.width);
		viewport.height = static_cast<float>(_size.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = _size;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertex_buffers[] = {main_mesh->vertexBuffer()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertex_buffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, main_mesh->indexBuffer(), 0, VK_INDEX_TYPE_UINT32);

		auto descriptor_sets = main_material->getDescriptorSet(_frame_index);

		vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				main_material->pipelineLayout(),
				0,
				descriptor_sets.size(),
				descriptor_sets.data(),
				0,
				nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(main_mesh->indexCount()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);
		require(vkEndCommandBuffer(commandBuffer));
	}

	void MainRenderPipeline::_update_uniform_buffer(uint32_t currentImage) {
		static auto start_time = std::chrono::high_resolution_clock::now();

		auto current_time = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), _size.width / (float) _size.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		_mapped_uniforms[currentImage].set_value(ubo);
	}

	VkFormat MainRenderPipeline::_find_depth_format() {
		return Graphics::DEFAULT->findSupportedFormat(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}
