#include <memory>
#include <vulkan/vulkan_core.h>

#include "RaytraceRenderPass.hpp"
#include "../types/Node.hpp"
#include "Shader.hpp"
#include "defs.hpp"
#include "imgui_impl_vulkan.h"

namespace vulkan {
	util::Result<RaytraceRenderPass::Ptr, KError> RaytraceRenderPass::create(
			VkExtent2D size)
	{
		auto result = std::unique_ptr<RaytraceRenderPass>(new RaytraceRenderPass());

		result->_size = size;

		result->_descriptor_pool = DescriptorPool::create();


		auto fence = Fence::create();
		TRY(fence);
		result->_pass_fence = std::move(fence.value());

		auto semaphore = Semaphore::create();
		TRY(semaphore);
		result->_pass_semaphore = std::move(semaphore.value());

		auto image = Image::create(
				result->_size.width, 
				result->_size.height,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
					| VK_IMAGE_USAGE_STORAGE_BIT
					| VK_IMAGE_USAGE_SAMPLED_BIT);
		TRY(image);
		result->_result_image = std::move(image.value());

		Graphics::DEFAULT->transition_image_layout(
				result->_result_image.value(),
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL,
				1);


		auto image_view = result->_result_image.create_image_view_full(
				VK_FORMAT_R8G8B8A8_SRGB, 
				VK_IMAGE_ASPECT_COLOR_BIT, 
				1);
		TRY(image_view);
		result->_result_image_view = std::move(image_view.value());

		auto buffer_res = MappedComputeUniform::create();
		TRY(buffer_res);
		result->_mapped_uniform = std::move(buffer_res.value());

		result->_imgui_descriptor_set = ImGui_ImplVulkan_AddTexture(
				Graphics::DEFAULT->main_texture_sampler(), 
				result->_result_image_view.value(), 
				VK_IMAGE_LAYOUT_GENERAL);
		auto command_info = VkCommandBufferAllocateInfo{};
		command_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_info.commandPool = Graphics::DEFAULT->command_pool();
		command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_info.commandBufferCount = 1;

		auto res = vkAllocateCommandBuffers(
				Graphics::DEFAULT->device(),
				&command_info,
				&result->_command_buffer);

		if (res != VK_SUCCESS) {
			return {res};
		}

		return {std::move(result)};
	}

	RaytraceRenderPass::~RaytraceRenderPass() {
		if (_pipeline_layout) {
			vkDestroyPipelineLayout(
					Graphics::DEFAULT->device(), 
					_pipeline_layout, 
					nullptr);
			_pipeline_layout = nullptr;
		}

		if (_pipeline) {
			vkDestroyPipeline(
					Graphics::DEFAULT->device(), 
					_pipeline, 
					nullptr);
			_pipeline = nullptr;
		}

		if (_imgui_descriptor_set) {
			ImGui_ImplVulkan_RemoveTexture(_imgui_descriptor_set);
			_imgui_descriptor_set = nullptr;
		}
	}

	RaytraceRenderPass::RaytraceRenderPass(RaytraceRenderPass &&other):
		_descriptor_set(std::move(other._descriptor_set))
	{
		_size = other._size;
		_result_image = std::move(other._result_image);
		_result_image_view = std::move(other._result_image_view);
		_pass_fence = std::move(other._pass_fence);
		_pass_semaphore = std::move(other._pass_semaphore);
		_descriptor_pool = std::move(other._descriptor_pool);
		
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;
	}

	RaytraceRenderPass& RaytraceRenderPass::operator=(RaytraceRenderPass&& other) {
		this->~RaytraceRenderPass();

		_size = other._size;
		_result_image = std::move(other._result_image);
		_result_image_view = std::move(other._result_image_view);
		_pass_fence = std::move(other._pass_fence);
		_pass_semaphore = std::move(other._pass_semaphore);
		_descriptor_pool = std::move(other._descriptor_pool);
		
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		return *this;
	}

	RaytraceRenderPass::RaytraceRenderPass():
	_pipeline_layout(nullptr),
	_pipeline(nullptr)
	{ }

	VkDescriptorSet RaytraceRenderPass::get_descriptor_set() {
		return _imgui_descriptor_set;
	}
	ImageView const &RaytraceRenderPass::image_view() {
		return _result_image_view;
	}

	void RaytraceRenderPass::submit(Node &node) {
		if (_descriptor_set.is_cleared()) {
			_create_descriptor_sets(node);
		}

		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		vkWaitForFences(Graphics::DEFAULT->device(), 1, &_pass_fence.value(), VK_TRUE, UINT64_MAX);

		vkResetFences(Graphics::DEFAULT->device(), 1, &_pass_fence.value());

		vkResetCommandBuffer(_command_buffer, 0);

		auto begin_info = VkCommandBufferBeginInfo{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(_command_buffer, &begin_info) != VK_SUCCESS) {
			LOG_ERROR << "Couldn't begin command buffer" << std::endl;
		}
		vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
		auto descriptor_set = _descriptor_set.descriptor_set(0);
		vkCmdBindDescriptorSets(
				_command_buffer,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				_pipeline_layout,
				0,
				1,
				&descriptor_set,
				0,
				nullptr);
		vkCmdDispatch(_command_buffer, 300, 300, 1);
		if (vkEndCommandBuffer(_command_buffer) != VK_SUCCESS) {
			LOG_ERROR << "Couldn't end command buffer" << std::endl;
		}

		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &_command_buffer;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = nullptr;

		auto res = vkQueueSubmit(Graphics::DEFAULT->graphics_queue(), 1, &submit_info, *_pass_fence);
		if (res != VK_SUCCESS) {
			LOG_ERROR << "Problem submitting queue" << std::endl;
		}
	}

	MappedComputeUniform &RaytraceRenderPass::current_uniform_buffer() {
		return _mapped_uniform;
	}

	util::Result<void, KError> RaytraceRenderPass::_create_descriptor_sets(Node &node) {
		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		descriptor_templates.push_back(DescriptorSetTemplate::create_image_target(
					0, 
					VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					_result_image_view));

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_mapped_uniform));

		descriptor_templates.push_back(DescriptorSetTemplate::create_storage_buffer(
					2, 
					VK_SHADER_STAGE_COMPUTE_BIT, 
					node.mesh().vertex_buffer(), 
					node.mesh().vertex_buffer_range()));

		descriptor_templates.push_back(DescriptorSetTemplate::create_storage_buffer(
					3, 
					VK_SHADER_STAGE_COMPUTE_BIT, 
					node.mesh().index_buffer(), 
					node.mesh().index_buffer_range()));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				1,
				_descriptor_pool);

		TRY(descriptor_sets);
		_descriptor_set = std::move(descriptor_sets.value());

		auto compute_shader = Shader::from_env_file("src/shaders/default_shader.comp.spv");

		auto compute_shader_stage_info = VkPipelineShaderStageCreateInfo{};
		compute_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compute_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_stage_info.module = compute_shader.shader_module();
		compute_shader_stage_info.pName = "main";

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = _descriptor_set.layout_ptr();

		auto res = vkCreatePipelineLayout(Graphics::DEFAULT->device(), &pipeline_layout_info, nullptr, &_pipeline_layout);
		if (res != VK_SUCCESS) {
			return {res};
		}

		auto pipeline_info = VkComputePipelineCreateInfo{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_info.layout = _pipeline_layout;
		pipeline_info.stage = compute_shader_stage_info;

		res = vkCreateComputePipelines(
				Graphics::DEFAULT->device(),
				VK_NULL_HANDLE,
				1,
				&pipeline_info,
				nullptr,
				&_pipeline);
		if (res != VK_SUCCESS) {
			return {res};
		}

		return {};
	}
}
