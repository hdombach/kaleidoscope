#include <array>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "PrevPass.hpp"
#include "PrevPassMaterial.hpp"
#include "PrevPassNode.hpp"
#include "Scene.hpp"
#include "Uniforms.hpp"
#include "defs.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "../util/log.hpp"
#include "../util/file.hpp"
#include "imgui_impl_vulkan.h"

namespace vulkan {
	/************************ Observers *********************************/
	PrevPass::MeshObserver::MeshObserver(PrevPass &render_pass):
		_render_pass(&render_pass)
	{}

	void PrevPass::MeshObserver::obs_create(uint32_t id) {
		_render_pass->mesh_create(id);
	}

	void PrevPass::MeshObserver::obs_update(uint32_t id) {
		_render_pass->mesh_update(id);
	}

	void PrevPass::MeshObserver::obs_remove(uint32_t id) {
		_render_pass->mesh_remove(id);
	}

	PrevPass::MaterialObserver::MaterialObserver(PrevPass &render_pass):
		_render_pass(&render_pass)
	{}

	void PrevPass::MaterialObserver::obs_create(uint32_t id) {
		_render_pass->material_create(id);
	}

	void PrevPass::MaterialObserver::obs_update(uint32_t id) {
		_render_pass->material_update(id);
	}

	void PrevPass::MaterialObserver::obs_remove(uint32_t id) {
		_render_pass->material_remove(id);
	}

	PrevPass::NodeObserver::NodeObserver(PrevPass &render_pass):
		_render_pass(&render_pass)
	{}

	void PrevPass::NodeObserver::obs_create(uint32_t id) {
		_render_pass->node_create(id);
	}

	void PrevPass::NodeObserver::obs_update(uint32_t id) {
		_render_pass->node_update(id);
	}

	void PrevPass::NodeObserver::obs_remove(uint32_t id) {
		_render_pass->node_remove(id);
	}

	/************************ PreviewRenderPass *********************************/

	util::Result<PrevPass::Ptr, KError> PrevPass::create(
			Scene &scene,
			VkExtent2D size)
	{
		auto result = std::unique_ptr<PrevPass>(
				new PrevPass(scene, size));
		TRY(result->_create_sync_objects());
		result->_create_command_buffers();
		result->_descriptor_pool = DescriptorPool::create();

		{
			auto res = result->_create_render_pass();
			TRY(res);
		}

		{
			auto res = result->_create_images();
			TRY(res);
		}

		{
			auto res = result->_create_overlay_pipeline();
			TRY(res);
		}

		{
			auto res = result->_create_de_pipeline();
			TRY(res);
		}

		/* create descriptor sets */
		{
			auto buffer = MappedPrevPassUniform::create();
			TRY(buffer);
			result->_mapped_uniform = std::move(buffer.value());
		}

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();
		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					0, 
					VK_SHADER_STAGE_VERTEX_BIT, 
					result->_mapped_uniform));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates, 
				1, 
				result->_descriptor_pool);
		TRY(descriptor_sets);
		result->_global_descriptor_set = std::move(descriptor_sets.value());

		result->_mesh_observer = MeshObserver(*result);
		result->_material_observer = MaterialObserver(*result);
		result->_node_observer = NodeObserver(*result);

		return result;
	}

	void PrevPass::destroy() {
		LOG_MEMORY << "Deconstructing main render pipeline" << std::endl;

		_cleanup_images();
		_nodes.clear();
		_meshes.clear();
		_materials.clear();
		_global_descriptor_set.destroy();
		_destroy_render_pass();
		_destroy_overlay_pipeline();
		_destroy_de_pipeline();

		_mapped_uniform.destroy();
		
		_fence.destroy();
		_semaphore.destroy();
	}

	PrevPass::~PrevPass() {
		destroy();
	}

	VkSemaphore PrevPass::render(
			std::vector<Node::Ptr> &nodes,
			types::Camera const &camera,
			VkSemaphore semaphore)
	{
		_fence.wait();
		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		_fence.reset();

		auto command_buffer = _command_buffer;

		vkResetCommandBuffer(command_buffer, 0);

		auto begin_info = VkCommandBufferBeginInfo{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;

		require(vkBeginCommandBuffer(command_buffer, &begin_info));

		auto render_pass_info = VkRenderPassBeginInfo{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = _render_pass;
		render_pass_info.framebuffer = _framebuffer;
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = _size;

		auto clear_values = std::array<VkClearValue, 3>{};
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clear_values[1].depthStencil = {1.0f, 0};
		clear_values[2].color = {{0}};

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

		auto uniform = GlobalPrevPassUniform{};
		uniform.camera_transformation = camera.gen_raster_mat();
		current_uniform_buffer().set_value(uniform);


		//TODO: pass the filter view
		for (auto &node : nodes) {
			if (!node) continue;
			auto &mesh = _meshes[node->mesh().id()];
			auto &material = _materials[node->material().id()];
			auto &prev_node = _nodes[node->id()];

			if (mesh.is_de()) continue;

			auto size = glm::vec2{_size.width, _size.height};
			/*node.material().preview_impl()->update_uniform(
					_frame_index, 
					node.position(), 
					size);*/
			vkCmdBindPipeline(
					command_buffer, 
					VK_PIPELINE_BIND_POINT_GRAPHICS, 
					material.pipeline());

			VkBuffer vertex_buffers[] = {mesh.vertex_buffer()};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

			auto descriptor_sets = std::array<VkDescriptorSet, 2>{
				global_descriptor_set(0),
				prev_node.descriptor_set().descriptor_set(0),
			};

			vkCmdBindDescriptorSets(
					command_buffer, 
					VK_PIPELINE_BIND_POINT_GRAPHICS, 
					material.pipeline_layout(),
					0,
					descriptor_sets.size(),
					descriptor_sets.data(),
					0,
					nullptr);

			vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.index_count()), 1, 0, 0, 0);
		}

		vkCmdEndRenderPass(command_buffer);

		// Overlay
		{
			auto uniform = OverlayUniform{};
			uniform.selected_node = _scene->selected_node();
			_mapped_overlay_uniform.set_value(uniform);

			vkCmdBindPipeline(
					command_buffer,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					_overlay_pipeline);

			auto descriptor_set = _overlay_descriptor_set.descriptor_set(0);

			vkCmdBindDescriptorSets(
					command_buffer,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					_overlay_pipeline_layout,
					0,
					1,
					&descriptor_set,
					0,
					nullptr);

			vkCmdDispatch(_command_buffer, _size.width, _size.height, 1);
		}

		// DE
		{
			auto uniform = ComputeUniform{};
			uniform.camera_rotation = camera.gen_rotate_mat();
			uniform.camera_translation = glm::vec4(camera.position, 0.0);
			uniform.aspect = static_cast<float>(camera.width) / static_cast<float>(camera.height);
			uniform.fovy = camera.fovy;
			_mapped_de_uniform.set_value(uniform);

			vkCmdBindPipeline(
					command_buffer,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					_de_pipeline);

			auto descriptor_set = _de_descriptor_set.descriptor_set(0);

			vkCmdBindDescriptorSets(
					command_buffer,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					_de_pipeline_layout,
					0,
					1,
					&descriptor_set,
					0,
					nullptr);

			vkCmdDispatch(_command_buffer, _size.width, _size.height, 1);
		}

		require(vkEndCommandBuffer(command_buffer));

		VkSemaphore finish_semaphore = _semaphore.get();

		auto wait_stages = std::array<VkPipelineStageFlags, 2>{VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submit_info.pWaitDstStageMask = wait_stages.data();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &_command_buffer;
		if (semaphore) {
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &semaphore;
		}
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &finish_semaphore;

		require(vkQueueSubmit(Graphics::DEFAULT->graphics_queue(), 1, &submit_info, _fence.get()));

		return finish_semaphore;
	}

	void PrevPass::resize(VkExtent2D size) {
		if (size.width == _size.width && size.height == _size.height) return;
		Graphics::DEFAULT->wait_idle();
		_cleanup_images();
		_size = size;
		auto res = _create_images();
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}

		_destroy_overlay_pipeline();
		res = _create_overlay_pipeline();
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}

		_destroy_de_pipeline();
		res = _create_de_pipeline();
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}
	}

	VkExtent2D PrevPass::size() const {
		return _size;
	}

	VkDescriptorSet PrevPass::imgui_descriptor_set() {
		return _imgui_descriptor_set;
	}

	ImageView const &PrevPass::image_view() {
		return _color_image_view;
	}
	VkRenderPass PrevPass::render_pass() {
		return _render_pass;
	}
	MappedPrevPassUniform &PrevPass::current_uniform_buffer() {
		return _mapped_uniform;
	}

	void PrevPass::mesh_create(uint32_t id) {
		LOG_DEBUG << "Creating preview mesh" << std::endl;
		while (id + 1 > _meshes.size()) {
			_meshes.push_back(PrevPassMesh());
		}
		if (auto mesh = PrevPassMesh::create(*_scene, _scene->resource_manager().get_mesh(id))) {
			_meshes[id] = std::move(mesh.value());
		} else {
			LOG_ERROR << "Couldn't create preview mesh: " << mesh.error().desc() << std::endl;
		}
	}

	void PrevPass::mesh_update(uint32_t id) { }

	void PrevPass::mesh_remove(uint32_t id) {
		_meshes[id].destroy();
	}

	void PrevPass::material_create(uint32_t id) {
		while (id + 1 > _materials.size()) {
			_materials.push_back(PrevPassMaterial());
		}
		if (auto material = PrevPassMaterial::create(*_scene, *this, _scene->resource_manager().get_material(id))) {
			_materials[id] = std::move(material.value());
		} else {
			LOG_ERROR << "Couldn't create preview material: " << material.error().desc() << ", " << material.error().content() << std::endl;
		}
	}

	void PrevPass::material_update(uint32_t id) { }

	void PrevPass::material_remove(uint32_t id) {
		_materials[id].destroy();
	}

	void PrevPass::node_create(uint32_t id) {
		while (id + 1 > _nodes.size()) {
			_nodes.push_back(PrevPassNode());
		}
		if (auto node = PrevPassNode::create(*_scene, *this, _scene->get_node(id))) {
			_nodes[id] = std::move(node.value());
		} else {
			LOG_ERROR << "Couldn't create preview node: " << node.error().desc() << std::endl;
		}
	}

	void PrevPass::node_update(uint32_t id) {
		_nodes[id].update();
	}

	void PrevPass::node_remove(uint32_t id) {
		_nodes[id].destroy();
	}

	PrevPass::PrevPass(Scene &scene, VkExtent2D size):
		_scene(&scene),
		_size(size)
	{
	}

	util::Result<void, KError> PrevPass::_create_render_pass() {
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

		auto node_attachment = VkAttachmentDescription{};
		node_attachment.format = _NODE_IMAGE_FORMAT;
		node_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		node_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		node_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		node_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		node_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		node_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		node_attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto node_attachment_ref = VkAttachmentReference{};
		node_attachment_ref.attachment = 2;
		node_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto color_attachment_refs = std::array<VkAttachmentReference, 2>{
			color_attachment_ref,
			node_attachment_ref,
		};

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = color_attachment_refs.size();
		subpass.pColorAttachments = color_attachment_refs.data();
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		auto attachments = std::array<VkAttachmentDescription, 3>{
			color_attachment,
			depth_attachment,
			node_attachment,
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
				&_render_pass);

		if (res != VK_SUCCESS) {
			return {res};
		}

		return {};
	}

	void PrevPass::_destroy_render_pass() {
		if (_render_pass) {
			vkDestroyRenderPass(Graphics::DEFAULT->device(), _render_pass, nullptr);
			_render_pass = nullptr;
		}
	}

	util::Result<void, KError> PrevPass::_create_sync_objects() {
		if (auto semaphore = Semaphore::create()) {
			_semaphore = std::move(semaphore.value());
		} else {
			return {semaphore.error()};
		}

		if (auto fence = Fence::create()) {
			_fence = std::move(fence.value());
		} else {
			return {fence.error()};
		}

		return {};
	}

	void PrevPass::_create_command_buffers() {
		auto alloc_info = VkCommandBufferAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = Graphics::DEFAULT->command_pool();
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		require(vkAllocateCommandBuffers(Graphics::DEFAULT->device(), &alloc_info, &_command_buffer));
	}

	util::Result<void, KError> PrevPass::_create_images() {
		_cleanup_images();
		/* create depth resources */
		{
			auto image = Image::create(
					_size.width,
					_size.height,
					_depth_format(),
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
						| VK_IMAGE_USAGE_SAMPLED_BIT);
			TRY(image);
			_depth_image = std::move(image.value());

			auto image_view = _depth_image.create_image_view_full(
					_depth_format(), 
					VK_IMAGE_ASPECT_DEPTH_BIT, 
					1);
			TRY(image_view);
			_depth_image_view = std::move(image_view.value());

			Graphics::DEFAULT->transition_image_layout(
					_depth_image.value(),
					_depth_format(),
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					1);
		}

		/* Create main color resources */
		{
			auto image = Image::create(
					_size.width,
					_size.height,
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
						| VK_IMAGE_USAGE_STORAGE_BIT
						| VK_IMAGE_USAGE_SAMPLED_BIT);
			TRY(image);
			_color_image = std::move(image.value());

			auto image_view = _color_image.create_image_view_full(
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_ASPECT_COLOR_BIT,
					1);
			TRY(image_view);
			_color_image_view = std::move(image_view.value());

			Graphics::DEFAULT->transition_image_layout(
					_color_image.value(),
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL,
					1);
		}

		/* Create node resources */
		{
			auto image = Image::create(
					_size.width,
					_size.height,
					_NODE_IMAGE_FORMAT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
						| VK_IMAGE_USAGE_STORAGE_BIT
						| VK_IMAGE_USAGE_SAMPLED_BIT);
			TRY(image);
			_node_image = std::move(image.value());

			auto image_view = _node_image.create_image_view_full(
					_NODE_IMAGE_FORMAT,
					VK_IMAGE_ASPECT_COLOR_BIT,
					1);
			TRY(image_view);
			_node_image_view = std::move(image_view.value());

			Graphics::DEFAULT->transition_image_layout(
					_node_image.value(), 
					_NODE_IMAGE_FORMAT, 
					VK_IMAGE_LAYOUT_UNDEFINED, 
					VK_IMAGE_LAYOUT_GENERAL, 
					1);
		}

		auto attachments = std::array<VkImageView, 3>{
			_color_image_view.value(),
			_depth_image_view.value(),
			_node_image_view.value(),
		};

		auto framebuffer_info = VkFramebufferCreateInfo{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = _render_pass;
		framebuffer_info.attachmentCount = attachments.size();
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = _size.width;
		framebuffer_info.height = _size.height;
		framebuffer_info.layers = 1;

		auto res = vkCreateFramebuffer(
				Graphics::DEFAULT->device(),
				&framebuffer_info,
				nullptr,
				&_framebuffer);

		_imgui_descriptor_set = ImGui_ImplVulkan_AddTexture(
				Graphics::DEFAULT->main_texture_sampler(),
				_color_image_view.value(),
				VK_IMAGE_LAYOUT_GENERAL);


		return {};
	}

	void PrevPass::_cleanup_images() {
		_depth_image.destroy();
		_depth_image_view.destroy();
		_color_image_view.destroy();
		_color_image.destroy();

		vkDestroyFramebuffer(Graphics::DEFAULT->device(), _framebuffer, nullptr);
		_framebuffer = nullptr;

		ImGui_ImplVulkan_RemoveTexture(_imgui_descriptor_set);
		_imgui_descriptor_set = nullptr;
	}

	VkFormat PrevPass::_depth_format() {
		return Graphics::DEFAULT->find_supported_format(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	util::Result<void, KError> PrevPass::_create_overlay_pipeline() {
		auto buffer = MappedOverlayUniform::create();
		TRY(buffer);
		_mapped_overlay_uniform = std::move(buffer.value());

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();
		descriptor_templates.push_back(DescriptorSetTemplate::create_image_target(
					0,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_color_image_view));
		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_mapped_overlay_uniform));
		descriptor_templates.push_back(DescriptorSetTemplate::create_image_target(
					2,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_node_image_view));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				1,
				_descriptor_pool);
		TRY(descriptor_sets);
		_overlay_descriptor_set = std::move(descriptor_sets.value());

		auto source_code = util::readEnvFile("assets/overlay_shader.comp");
		util::replace_substr(source_code, "/*OVERLAY_UNIFORM_CONTENT*/\n", OverlayUniform::declaration_content());
		auto compute_shader = Shader::from_source_code(source_code, Shader::Type::Compute);
		if (!compute_shader) {
			util::add_strnum(source_code);
			LOG_DEBUG << "\n" << source_code << std::endl;
			LOG_ERROR << compute_shader.error() << std::endl;
			return compute_shader.error();
		}

		auto compute_shader_stage_info = VkPipelineShaderStageCreateInfo{};
		compute_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compute_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_stage_info.module = compute_shader.value().shader_module();
		compute_shader_stage_info.pName = "main";

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = _overlay_descriptor_set.layout_ptr();

		auto res = vkCreatePipelineLayout(
				Graphics::DEFAULT->device(),
				&pipeline_layout_info,
				nullptr,
				&_overlay_pipeline_layout);
		if (res != VK_SUCCESS) {
			return {res};
		}

		auto pipeline_info = VkComputePipelineCreateInfo{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_info.layout = _overlay_pipeline_layout;
		pipeline_info.stage = compute_shader_stage_info;

		res = vkCreateComputePipelines(
				Graphics::DEFAULT->device(),
				VK_NULL_HANDLE,
				1,
				&pipeline_info,
				nullptr,
				&_overlay_pipeline);
		if (res != VK_SUCCESS) {
			return {res};
		}

		return {};
	}

	void PrevPass::_destroy_overlay_pipeline() {
		vkDestroyPipelineLayout(
				Graphics::DEFAULT->device(),
				_overlay_pipeline_layout, 
				nullptr);
		vkDestroyPipeline(
				Graphics::DEFAULT->device(), 
				_overlay_pipeline, 
				nullptr);
		_overlay_descriptor_set.destroy();
	}

	util::Result<void, KError> PrevPass::_create_de_pipeline() {
		auto buffer = MappedComputeUniform::create();
		TRY(buffer);
		_mapped_de_uniform = std::move(buffer.value());
		
		auto descriptor_templates = std::vector<DescriptorSetTemplate>();
		descriptor_templates.push_back(DescriptorSetTemplate::create_image_target(
					0,
					VK_SHADER_STAGE_COMPUTE_BIT, 
					_color_image_view));

		descriptor_templates.push_back(DescriptorSetTemplate::create_image_target(
					1, 
					VK_SHADER_STAGE_COMPUTE_BIT, 
					_node_image_view));

		descriptor_templates.push_back(DescriptorSetTemplate::create_image(
					2, 
					VK_SHADER_STAGE_COMPUTE_BIT, 
					_depth_image_view));

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					3,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_mapped_de_uniform));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				1,
				_descriptor_pool);
		TRY(descriptor_sets);
		_de_descriptor_set = std::move(descriptor_sets.value());

		auto source_code = util::readEnvFile("assets/de_shader.comp");
		util::replace_substr(source_code, "/*UNIFORM_DECL*/\n", ComputeUniform::declaration());
		auto compute_shader = Shader::from_source_code(source_code, Shader::Type::Compute);
		if (!compute_shader) {
			util::add_strnum(source_code);
			LOG_DEBUG << "\n" << source_code << std::endl;
			LOG_DEBUG << compute_shader.error() << std::endl;
			return compute_shader.error();
		}

		auto compute_shader_stage_info = VkPipelineShaderStageCreateInfo{};
		compute_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compute_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_stage_info.module = compute_shader.value().shader_module();
		compute_shader_stage_info.pName = "main";

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = _de_descriptor_set.layout_ptr();

		auto res = vkCreatePipelineLayout(
				Graphics::DEFAULT->device(),
				&pipeline_layout_info,
				nullptr,
				&_de_pipeline_layout);
		if (res != VK_SUCCESS) {
			return {res};
		}

		auto pipeline_info = VkComputePipelineCreateInfo{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipeline_info.layout = _de_pipeline_layout;
		pipeline_info.stage = compute_shader_stage_info;

		res = vkCreateComputePipelines(
				Graphics::DEFAULT->device(),
				VK_NULL_HANDLE,
				1,
				&pipeline_info,
				nullptr,
				&_de_pipeline);
		if (res !=VK_SUCCESS) {
			return {res};
		}

		return {};
	}

	void PrevPass::_destroy_de_pipeline() {
		vkDestroyPipelineLayout(
				Graphics::DEFAULT->device(),
				_de_pipeline_layout,
				nullptr);
		vkDestroyPipeline(
				Graphics::DEFAULT->device(),
				_de_pipeline,
				nullptr);
		_de_descriptor_set.destroy();
	}
}
