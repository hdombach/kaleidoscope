#include <array>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "codegen/TemplGen.hpp"
#include "imgui_impl_vulkan.h"

#include "PrevPass.hpp"
#include "PrevPassMaterial.hpp"
#include "PrevPassNode.hpp"
#include "util/BaseError.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/Uniforms.hpp"
#include "vulkan/graphics.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/file.hpp"
#include "types/Mesh.hpp"
#include "../PassUtil.hpp"
#include "vulkan/Texture.hpp"

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

	util::Result<PrevPass::Ptr, PrevPass::Error> PrevPass::create(
			Scene &scene,
			VkExtent2D size)
	{
		auto result = std::unique_ptr<PrevPass>(
				new PrevPass(scene, size));
		if (auto err = result->_create_sync_objects().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create sync objects", err.value());
		}
		result->_create_command_buffers();
		result->_descriptor_pool = DescriptorPool::create();

		if (auto err = result->_create_images().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create images", err.value());
		}
		if (auto err = result->_create_shared_descriptor_set().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create shared descriptor set", err.value());
		}

		if (auto err = result->_create_prim_render_pass().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create prim render pass", err.value());
		}
		if (auto err = result->_create_de_render_pass().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create de render pass", err.value());
		}
		if (auto err = result->_create_overlay_descriptor_set().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create overlay descriptor set", err.value());
		}
		if (auto err = result->_create_overlay_pipeline().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create overlay pipeline", err.value());
		}
		if (auto err = result->_create_framebuffers().move_or()) {
			return Error(ErrorType::VULKAN, "Could not create framebuffers", err.value());
		}

		result->_de_buf_dirty_bit = true;
		result->_de_pipe_dirty_bit = true;;

		result->_mesh_observer = MeshObserver(*result);
		result->_material_observer = MaterialObserver(*result);
		result->_node_observer = NodeObserver(*result);

		return result;
	}

	void PrevPass::destroy() {
		log_memory() << "Deconstructing main render pipeline" << std::endl;

		_cleanup_images();
		_destroy_framebuffers();
		_nodes.clear();
		_meshes.clear();
		_materials.clear();
		_destroy_shared_descriptor_set();
		_destroy_prim_render_pass();
		_destroy_de_buffers();
		_destroy_de_render_pass();
		_destroy_de_pipeline();
		_destroy_de_descriptor_set();
		_destroy_overlay_pipeline();
		_overlay_descriptor_set.destroy();
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
		if (_de_buf_dirty_bit) {
			if (auto err = _create_de_buffers().move_or()) {
				log_error() << err.value() << std::endl;
			}
			if (auto err = _create_de_descriptor_set().move_or()) {
				log_error() << err.value() << std::endl;
			}
			_de_buf_dirty_bit = false;
		}

		if (_de_pipe_dirty_bit) {
			if (auto err = _create_de_pipeline().move_or()) {
				log_error() << err.value() << std::endl;
			}
			_de_pipe_dirty_bit = false;
		}

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

		util::require(vkBeginCommandBuffer(command_buffer, &begin_info));

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

		// Primary render of meshes
		{
			auto render_pass_info = VkRenderPassBeginInfo{};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_info.renderPass = _prim_render_pass;
			render_pass_info.framebuffer = _prim_framebuffer;
			render_pass_info.renderArea.offset = {0, 0};
			render_pass_info.renderArea.extent = _size;

			auto clear_values = std::array<VkClearValue, 4>{};
			clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
			clear_values[1].depthStencil = {1.0f, 0};
			clear_values[2].color = {{0}};
			clear_values[3].color = {{1.0}};

			render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_info.pClearValues = clear_values.data();

			vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

			_prim_uniform.set_value(GlobalPrevPassUniform::create(camera));

			//TODO: pass the filter view
			for (auto &node : nodes) {
				if (!node) continue;
				if (node->type() != vulkan::Node::Type::Object) continue;
				auto &mesh = _meshes[node->mesh().id()];
				auto &material = _materials[node->material().id()];
				auto &prev_node = _nodes[node->id()];

				if (mesh.is_de()) continue;
				if (material->pipeline() == nullptr) continue;

				auto size = glm::vec2{_size.width, _size.height};
				/*node.material().preview_impl()->update_uniform(
						_frame_index, 
						node.position(), 
						size);*/
				vkCmdBindPipeline(
						command_buffer, 
						VK_PIPELINE_BIND_POINT_GRAPHICS, 
						material->pipeline());

				VkBuffer vertex_buffers[] = {mesh.vertex_buffer()};
				VkDeviceSize offsets[] = {0};
				vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
				vkCmdBindIndexBuffer(command_buffer, mesh.index_buffer(), 0, VK_INDEX_TYPE_UINT32);

				auto descriptor_sets = std::array<VkDescriptorSet, 2>{
					shared_descriptor_set(),
					prev_node.descriptor_set().descriptor_set(0),
				};

				vkCmdBindDescriptorSets(
						command_buffer, 
						VK_PIPELINE_BIND_POINT_GRAPHICS, 
						material->pipeline_layout(),
						0,
						descriptor_sets.size(),
						descriptor_sets.data(),
						0,
						nullptr);

				vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(mesh.index_count()), 1, 0, 0, 0);
			}

			vkCmdEndRenderPass(command_buffer);
		}

		// de render pass
		if (1) {
			auto render_pass_info = VkRenderPassBeginInfo{};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_info.renderPass = _de_render_pass;
			render_pass_info.framebuffer = _de_framebuffer;
			render_pass_info.renderArea.offset = {0, 0};
			render_pass_info.renderArea.extent = _size;

			auto clear_values = std::array<VkClearValue, 3>{};
			clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
			clear_values[1].depthStencil = {1.0f, 0};
			clear_values[2].color = {{0}};

			render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_info.pClearValues = clear_values.data();

			vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

			log_assert(_de_pipeline, "DE pipeline does not exist");
			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _de_pipeline);

			auto descriptor_sets = std::array<VkDescriptorSet, 2>{
				shared_descriptor_set(),
				_de_descriptor_set.descriptor_set(0),
			};

			vkCmdBindDescriptorSets(
					command_buffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					_de_pipeline_layout,
					0,
					descriptor_sets.size(),
					descriptor_sets.data(),
					0,
					nullptr);

			vkCmdDraw(command_buffer, 6, 1, 0, 0);

			vkCmdEndRenderPass(command_buffer);
		}

		// Overlay
		if (1) {
			auto uniform = OverlayUniform{};
			uniform.selected_node = _scene->selected_node();
			_mapped_overlay_uniform.set_value(uniform);

			log_assert(_overlay_pipeline, "Overlay pipeline does not exist");
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

		util::require(vkEndCommandBuffer(command_buffer));

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

		util::require(vkQueueSubmit(Graphics::DEFAULT->graphics_queue(), 1, &submit_info, _fence.get()));

		return finish_semaphore;
	}

	void PrevPass::resize(VkExtent2D size) {
		if (size.width == _size.width && size.height == _size.height) return;
		Graphics::DEFAULT->wait_idle();
		_cleanup_images();
		_destroy_framebuffers();
		_size = size;

		if (auto err = _create_images().move_or()) {
			log_error() << err.value() << std::endl;
		}

		if (auto err = _create_framebuffers().move_or()) {
			log_error() << err.value() << std::endl;
		}

		if (auto err = _create_overlay_descriptor_set().move_or()) {
			log_error() << err.value() << std::endl;
		}

		if (auto err = _create_de_descriptor_set().move_or()) {
			log_error() << err.value() << std::endl;
		}
	}

	VkExtent2D PrevPass::size() const {
		return _size;
	}

	VkDescriptorSet PrevPass::imgui_descriptor_set() {
		return _imgui_descriptor_set;
	}

	VkImageView PrevPass::image_view() {
		return _color_image.image_view();
	}
	VkRenderPass PrevPass::render_pass() {
		return _prim_render_pass;
	}

	void PrevPass::mesh_create(uint32_t id) {
		if (auto mesh = PrevPassMesh::create(*_scene, _scene->resource_manager().get_mesh(id))) {
			log_assert(_meshes.insert(std::move(mesh.value())), "Duplicated mesh in PrevPass");
		} else {
			std::cerr << std::endl << mesh.error() << std::endl;
		}

		_de_buf_dirty_bit = true;
	}

	void PrevPass::mesh_update(uint32_t id) { }

	void PrevPass::mesh_remove(uint32_t id) {
		_meshes[id].destroy();
	}

	void PrevPass::material_create(uint32_t id) {
		if (auto material = PrevPassMaterial::create(
			*_scene,
			*this,
			_scene->resource_manager().get_material(id))
		) {
			log_assert(_materials.insert(std::move(material.value())), "Duplicated material in PrevPass");

			_de_buf_dirty_bit = true;
		} else {
			if (!material.has_value()) {
				log_error() << std::endl << material.error() << std::endl;
			}
		}
	}

	void PrevPass::material_update(uint32_t id) { }

	void PrevPass::material_remove(uint32_t id) {
		_materials[id]->destroy();
	}

	void PrevPass::node_create(uint32_t id) {
		auto node = _scene->get_node(id);
		if (node->type() != vulkan::Node::Type::Object) return;

		if (auto prev_node = PrevPassNode::create(*_scene, *this, node)) {
			log_assert(_nodes.insert(std::move(prev_node.value())), "Duplicated node in PrevPass");
		} else {
			log_error() << prev_node.error() << std::endl;
		}

		_de_buf_dirty_bit = true;
	}

	void PrevPass::node_update(uint32_t id) {
		if (!_nodes.contains(id)) return;

		_nodes[id].update();

		_de_buf_dirty_bit = true;
	}

	void PrevPass::node_remove(uint32_t id) {
		if (!_nodes.contains(id)) return;

		_nodes[id].destroy();

		_de_buf_dirty_bit = true;
	}

	PrevPass::PrevPass(Scene &scene, VkExtent2D size):
		_scene(&scene),
		_size(size)
	{
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_prim_render_pass() {
		_destroy_de_render_pass();

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

		auto depth_buf_attachment = VkAttachmentDescription{};
		depth_buf_attachment.format = _DEPTH_BUF_IMAGE_FORMAT;
		depth_buf_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_buf_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_buf_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depth_buf_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_buf_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_buf_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_buf_attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto depth_buf_attachment_ref = VkAttachmentReference{};
		depth_buf_attachment_ref.attachment = 3;
		depth_buf_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


		auto color_attachment_refs = std::array<VkAttachmentReference, 3>{
			color_attachment_ref,
			node_attachment_ref,
			depth_buf_attachment_ref,
		};

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = color_attachment_refs.size();
		subpass.pColorAttachments = color_attachment_refs.data();
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		auto attachments = std::array<VkAttachmentDescription, 4>{
			color_attachment,
			depth_attachment,
			node_attachment,
			depth_buf_attachment,
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
				&_prim_render_pass);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create render pass", VkError(res));
		}

		return {};
	}

	void PrevPass::_destroy_prim_render_pass() {
		if (_prim_render_pass) {
			vkDestroyRenderPass(Graphics::DEFAULT->device(), _prim_render_pass, nullptr);
			_prim_render_pass = nullptr;
		}
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_overlay_descriptor_set() {
		if (auto err = MappedOverlayUniform::create().move_or(_mapped_overlay_uniform)) {
			return Error(ErrorType::VULKAN, "Could not create mapped overlay uniform", {err.value()});
		}

		auto bindings = std::vector<VkDescriptorSetLayoutBinding>();
		bindings.push_back(descriptor_layout_image_target(VK_SHADER_STAGE_COMPUTE_BIT, 1));
		bindings.push_back(descriptor_layout_uniform(VK_SHADER_STAGE_COMPUTE_BIT));
		bindings.push_back(descriptor_layout_image_target(VK_SHADER_STAGE_COMPUTE_BIT, 1));

		if (auto err = DescriptorSetLayout::create(bindings).move_or(_overlay_descriptor_set_layout)) {
			return Error(ErrorType::RESOURCE, "Could not create overlay descriptor set layout", err.value());
		}

		auto builder = _overlay_descriptor_set_layout.builder();
		if (auto err = builder.add_image_target(_color_image.image_view()).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not add color image target", {err.value()});
		}
		if (auto err = builder.add_uniform(_mapped_overlay_uniform).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not add uniform to descriptor set", {err.value()});
		}
		if (auto err = builder.add_image_target(_de_node_image.image_view()).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not add de node image target", {err.value()});
		}
		if (auto err = DescriptorSets::create(builder, descriptor_pool()).move_or(_overlay_descriptor_set)) {
			return Error(ErrorType::RESOURCE, "Could not create DescriptorSet", err.value());
		}

		return {};
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_overlay_pipeline() {
		_destroy_overlay_pipeline();

		auto codegen_args = cg::TemplObj{
			{"overlay_declarations", OverlayUniform::declaration_content}
		};
		auto source_code = util::readEnvFile("assets/shaders/preview_overlay.comp.cg");
		auto start = log_start_timer();
		source_code = cg::TemplGen::codegen(source_code, codegen_args, "preview_overlay.comp.cg").value();
		log_info() << "preview_overlay codegen took " << start << std::endl;
		auto compute_shader = Shader::from_source_code(source_code, Shader::Type::Compute);
		log_info() << "\n" << util::add_strnum(source_code) << std::endl;
		if (!compute_shader) {
			log_fatal_error() << "Overlay error " << compute_shader.error() << std::endl;
			return Error(ErrorType::VULKAN, "Could not create preview_overlay shader", compute_shader.error());
		}

		auto compute_shader_stage_info = VkPipelineShaderStageCreateInfo{};
		compute_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compute_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_stage_info.module = compute_shader.value().shader_module();
		compute_shader_stage_info.pName = "main";

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &_overlay_descriptor_set_layout.layout();

		auto res = vkCreatePipelineLayout(
				Graphics::DEFAULT->device(),
				&pipeline_layout_info,
				nullptr,
				&_overlay_pipeline_layout);
		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create overlay pipeline layout", VkError(res));
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
			return Error(ErrorType::VULKAN, "Could not create overlay compute pipeline", VkError(res));
		}

		return {};
	}

	void PrevPass::_destroy_overlay_pipeline() {
		if (_overlay_pipeline_layout) {
			vkDestroyPipelineLayout(
					Graphics::DEFAULT->device(),
					_overlay_pipeline_layout, 
					nullptr);
			_overlay_pipeline_layout = nullptr;
		}
		if (_overlay_pipeline) {
			vkDestroyPipeline(
					Graphics::DEFAULT->device(), 
					_overlay_pipeline, 
					nullptr);
			_overlay_pipeline = nullptr;
		}
	}

	//TODO unify this as well.
	std::vector<VkImageView> used_textures(
		types::ResourceManager::TextureContainer const &textures
	) {
		auto result = std::vector<VkImageView>();

		for (auto &t : textures.raw()) {
			if (t) {
				result.push_back(t->image_view());
			} else {
				result.push_back(nullptr);
			}
		}

		return result;
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_de_descriptor_set() {
		_destroy_de_descriptor_set();

		auto textures = used_textures(_scene->resource_manager().textures());

		auto bindings = std::vector<VkDescriptorSetLayoutBinding>();
		bindings.push_back(descriptor_layout_image(VK_SHADER_STAGE_FRAGMENT_BIT));
		bindings.push_back(descriptor_layout_image(VK_SHADER_STAGE_FRAGMENT_BIT));
		bindings.push_back(descriptor_layout_storage_buffer(VK_SHADER_STAGE_FRAGMENT_BIT));
		bindings.push_back(descriptor_layout_storage_buffer(VK_SHADER_STAGE_FRAGMENT_BIT));
		if (textures.size() > 0) {
			bindings.push_back(descriptor_layout_images(VK_SHADER_STAGE_FRAGMENT_BIT, textures.size()));
		}

		if (auto err = DescriptorSetLayout::create(bindings).move_or(_de_descriptor_set_layout)) {
			return Error(ErrorType::RESOURCE, "Could not create descriptor set layout", err.value());
		}

		auto builder = _de_descriptor_set_layout.builder();
		if (auto err = builder.add_image(_depth_buf_image.image_view(), VK_IMAGE_LAYOUT_GENERAL).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not add depth image", err.value());
		}
		if (auto err = builder.add_image(_node_image.image_view(), VK_IMAGE_LAYOUT_GENERAL).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not add node image", err.value());
		}
		if (auto err = builder.add_storage_buffer(_de_node_buffer).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not add de node buffer", err.value());
		}
		if (auto err = builder.add_storage_buffer(_de_material_buffer).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not de material buffer", err.value());
		}
		if (textures.size() > 0) {
			if (auto err = builder.add_image(textures).move_or()) {
				return Error(ErrorType::RESOURCE, "Could not add textures", err.value());
			}
		}
		if (auto err = DescriptorSets::create(builder, descriptor_pool()).move_or(_de_descriptor_set)) {
			return Error(ErrorType::RESOURCE, "Could not create de descriptor set", err.value());
		}

		return {};
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_de_render_pass() {
		_destroy_de_render_pass();
		auto color_attachment = VkAttachmentDescription{};
		color_attachment.format = _RESULT_IMAGE_FORMAT;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto node_attachment = VkAttachmentDescription{};
		node_attachment.format = _NODE_IMAGE_FORMAT;
		node_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		node_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		node_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		node_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		node_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		node_attachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		node_attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto color_attachment_ref = VkAttachmentReference{};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto node_attachment_ref = VkAttachmentReference{};
		node_attachment_ref.attachment = 1;
		node_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto attachment_refs = std::array<VkAttachmentReference, 2>{
			color_attachment_ref,
			node_attachment_ref,
		};

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = attachment_refs.size();
		subpass.pColorAttachments = attachment_refs.data();
		subpass.pDepthStencilAttachment = nullptr;

		auto attachments = std::array<VkAttachmentDescription, 2>{
			color_attachment,
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
				&_de_render_pass);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create de render pass", VkError(res));
		}

		return {};
	}

	void PrevPass::_destroy_de_descriptor_set() {
		_de_descriptor_set.destroy();
	}

	void PrevPass::_destroy_de_render_pass() {
		if (_de_render_pass) {
			vkDestroyRenderPass(Graphics::DEFAULT->device(), _de_render_pass, nullptr);
			_de_render_pass = nullptr;
		}
	}


	util::Result<void, PrevPass::Error> PrevPass::_create_de_pipeline() {
		_destroy_de_pipeline();

		auto vert_source_code = util::readEnvFile("assets/shaders/unit_square.vert");
		auto vert_shader = Shader::from_source_code(vert_source_code, Shader::Type::Vertex);
		if (!vert_shader) {
			log_info() << "\n" << util::add_strnum(vert_source_code) << std::endl;
			log_error() << vert_shader.error() << std::endl;
			return Error(ErrorType::VULKAN, "Could not create vert shader", vert_shader.error());
		}

		auto frag_source_code = _codegen_de();
		auto frag_shader = Shader::from_source_code(frag_source_code, Shader::Type::Fragment);
		if (!frag_shader) {
			log_info() << "\n" << util::add_strnum(frag_source_code) << std::endl;
			log_error() << frag_shader.error() << std::endl;
			return Error(ErrorType::VULKAN, "Could not create frag shader", frag_shader.error());
		}

		auto shader_stage_infos = std::array<VkPipelineShaderStageCreateInfo, 2>();

		shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stage_infos[0].module = vert_shader.value().shader_module();
		shader_stage_infos[0].pName = "main";

		shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stage_infos[1].module = frag_shader.value().shader_module();
		shader_stage_infos[1].pName = "main";

		auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{};

		auto vertex_binding_description = VkVertexInputBindingDescription{};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(glm::vec4);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		auto vertex_attribute_descriptions = std::array<VkVertexInputAttributeDescription, 1>();
		vertex_attribute_descriptions[0].binding = 0;
		vertex_attribute_descriptions[0].location = 0;
		vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_attribute_descriptions[0].offset = 0;

		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &vertex_binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(
				vertex_attribute_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

		auto input_assembly = VkPipelineInputAssemblyStateCreateInfo{};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.width = (float) 100;
		viewport.height = (float) 100;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = VkExtent2D{100, 100};

		auto viewport_state = VkPipelineViewportStateCreateInfo{};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		auto rasterizer = VkPipelineRasterizationStateCreateInfo{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		auto multisampling = VkPipelineMultisampleStateCreateInfo{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		auto color_blend_attachment = VkPipelineColorBlendAttachmentState{};
		color_blend_attachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_TRUE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		auto node_blend_attachment = VkPipelineColorBlendAttachmentState{};
		node_blend_attachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		node_blend_attachment.blendEnable = VK_FALSE;


		auto color_attachments = std::array<VkPipelineColorBlendAttachmentState, 2>{
			color_blend_attachment,
			node_blend_attachment,
		};

		auto color_blending = VkPipelineColorBlendStateCreateInfo{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = color_attachments.size();
		color_blending.pAttachments = color_attachments.data();
		color_blending.blendConstants[0] = 0.0f;
		color_blending.blendConstants[1] = 0.0f;
		color_blending.blendConstants[2] = 0.0f;
		color_blending.blendConstants[3] = 0.0f;

		auto dynamic_states = std::array<VkDynamicState, 2>{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		auto dynamic_state = VkPipelineDynamicStateCreateInfo{};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
		dynamic_state.pDynamicStates = dynamic_states.data();

		auto descriptor_set_layouts = std::array<VkDescriptorSetLayout, 2>{
			shared_descriptor_set_layout(),
			_de_descriptor_set_layout.layout(),
		};

		auto dset_layout = shared_descriptor_set_layout();
		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = descriptor_set_layouts.size();
		pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		auto res = vkCreatePipelineLayout(
				Graphics::DEFAULT->device(),
				&pipeline_layout_info,
				nullptr,
				&_de_pipeline_layout);
		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create de pipeline layout", VkError(res));
		}

		auto depth_stencil = VkPipelineDepthStencilStateCreateInfo{};
		depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil.depthTestEnable = VK_TRUE;
		depth_stencil.depthWriteEnable = VK_TRUE;
		depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil.depthBoundsTestEnable = VK_FALSE;
		depth_stencil.minDepthBounds = 0.0f;
		depth_stencil.maxDepthBounds = 1.0f;
		depth_stencil.stencilTestEnable = VK_FALSE;
		depth_stencil.front = {};
		depth_stencil.back = {};


		auto pipeline_info = VkGraphicsPipelineCreateInfo{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = shader_stage_infos.size();
		pipeline_info.pStages = shader_stage_infos.data();
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		//pipeline_info.pDepthStencilState = &depth_stencil;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_state;
		pipeline_info.layout = _de_pipeline_layout;
		pipeline_info.renderPass = _de_render_pass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		res = vkCreateGraphicsPipelines(
				Graphics::DEFAULT->device(), 
				VK_NULL_HANDLE, 
				1,
				&pipeline_info, 
				nullptr, 
				&_de_pipeline);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create de pipeline", VkError(res));
		}

		return {};
	}

	void PrevPass::_destroy_de_pipeline() {
		if (_de_pipeline_layout) {
			vkDestroyPipelineLayout(
					Graphics::DEFAULT->device(), 
					_de_pipeline_layout, 
					nullptr);
			_de_pipeline_layout = nullptr;
		}
		if (_de_pipeline) {
			vkDestroyPipeline(
					Graphics::DEFAULT->device(), 
					_de_pipeline, 
					nullptr);
			_de_pipeline = nullptr;
		}
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_de_buffers() {
		auto nodes = std::vector<PrevPassNode::VImpl>();
		for (auto &node : _nodes.raw()) {
			if (node && node.is_de()) {
				nodes.push_back(node.vimpl());
			} else {
				nodes.push_back(PrevPassNode::VImpl::create_empty());
			}
		}

		if (nodes.empty()) {
			nodes.push_back(PrevPassNode::VImpl::create_empty());
		}

		if (auto err = StaticBuffer::create(nodes).move_or(_de_node_buffer)) {
			if (err->type() != vulkan::ErrorType::EMPTY_BUFFER) {
				log_error() << err.value() << std::endl;
			}
			return Error(ErrorType::RESOURCE, "Could not create de node buffer", err.value());
		}

		if (auto err = create_material_buffer(*_scene).move_or(_de_material_buffer)) {
			return Error(ErrorType::RESOURCE, "Could not create de material buffer", err.value());
		}

		return {};
	}

	void PrevPass::_destroy_de_buffers() {
		_de_node_buffer.destroy();
		_de_material_buffer.destroy();
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_images() {
		_cleanup_images();
		/* create depth resources */
		if (auto err = Image::create(
			_size,
			_depth_format(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).move_or(_depth_image)
		) {
			return Error(ErrorType::VULKAN, "Could not create depth image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_depth_image.image(),
			_depth_format(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			1
		);

		/* Create main color resources */
		if (auto err = Image::create(
			_size,
			_RESULT_IMAGE_FORMAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_STORAGE_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT).move_or(_color_image)
		) {
			return Error(ErrorType::VULKAN, "Could not create color image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_color_image.image(),
			_RESULT_IMAGE_FORMAT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			1
		);

		/* Create node resources */
		if (auto err = Image::create(
			_size,
			_NODE_IMAGE_FORMAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_STORAGE_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT).move_or(_node_image)
		) {
			return Error(ErrorType::VULKAN, "Could not create node image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_node_image.image(), 
			_NODE_IMAGE_FORMAT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_GENERAL, 
			1
		);

		/* Create de node images */
		if (auto err = Image::create(
			_size,
			_NODE_IMAGE_FORMAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_STORAGE_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT).move_or(_de_node_image)
		) {
			return Error(ErrorType::VULKAN, "Could not create de node image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_de_node_image.image(), 
			_NODE_IMAGE_FORMAT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_GENERAL, 
			1
		);

		/* depth buf */
		if (auto err = Image::create(
			_size,
			_DEPTH_BUF_IMAGE_FORMAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_STORAGE_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT).move_or(_depth_buf_image)
		) {
			return Error(ErrorType::VULKAN, "Could not create depth buf image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_depth_buf_image.image(), 
			_DEPTH_BUF_IMAGE_FORMAT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_GENERAL, 
			1
		);

		_imgui_descriptor_set = ImGui_ImplVulkan_AddTexture(
			*Graphics::DEFAULT->main_texture_sampler(),
			_color_image.image_view(),
			VK_IMAGE_LAYOUT_GENERAL
		);

		return {};
	}

	void PrevPass::_cleanup_images() {
		_depth_image.destroy();
		_color_image.destroy();
		_de_node_image.destroy();
		_depth_buf_image.destroy();

		ImGui_ImplVulkan_RemoveTexture(_imgui_descriptor_set);
		_imgui_descriptor_set = nullptr;
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_shared_descriptor_set() {
		{
			if (auto err = MappedPrevPassUniform::create().move_or(_prim_uniform)) {
				return Error(ErrorType::VULKAN, "Could not create prim uniform", err.value());
			}
		}

		auto descriptor_bindings = std::vector<VkDescriptorSetLayoutBinding>();
		descriptor_bindings.push_back(descriptor_layout_uniform(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));

		_shared_descriptor_set_layout = std::move(DescriptorSetLayout::create(descriptor_bindings).value());

		auto builder = _shared_descriptor_set_layout.builder();
		if (auto err = builder.add_uniform(_prim_uniform).move_or()) {
			return Error(ErrorType::VULKAN, "Could not add prim uniform", err.value());
		}

		if (auto err = DescriptorSets::create(builder, _descriptor_pool).move_or(_shared_descriptor_set)) {
			return Error(ErrorType::VULKAN, "Could not create shared desriptor set", err.value());
		}

		return {};
	}

	void PrevPass::_destroy_shared_descriptor_set() {
		_shared_descriptor_set.destroy();
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_framebuffers() {
		{
			auto attachments = std::array<VkImageView, 4>{
				_color_image.image_view(),
				_depth_image.image_view(),
				_node_image.image_view(),
				_depth_buf_image.image_view(),
			};

			auto framebuffer_info = VkFramebufferCreateInfo{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = _prim_render_pass;
			framebuffer_info.attachmentCount = attachments.size();
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = _size.width;
			framebuffer_info.height = _size.height;
			framebuffer_info.layers = 1;

			auto res = vkCreateFramebuffer(
				Graphics::DEFAULT->device(),
				&framebuffer_info,
				nullptr,
				&_prim_framebuffer
			);

			if (res != VK_SUCCESS) {
				return Error(ErrorType::VULKAN, "Could not create prim framebuffer", VkError(res));
			}
		}

		{
			auto attachments = std::array<VkImageView, 2>{
				_color_image.image_view(),
				_de_node_image.image_view(),
			};

			auto framebuffer_info = VkFramebufferCreateInfo{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = _de_render_pass;
			framebuffer_info.attachmentCount = attachments.size();
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = _size.width;
			framebuffer_info.height = _size.height;
			framebuffer_info.layers = 1;

			auto res = vkCreateFramebuffer(
				Graphics::DEFAULT->device(),
				&framebuffer_info,
				nullptr,
				&_de_framebuffer
			);

			if (res != VK_SUCCESS) {
				return Error(ErrorType::VULKAN, "Could not create de framebuffer", VkError(res));
			}

		}

		return {};
	}

	void PrevPass::_destroy_framebuffers() {
		vkDestroyFramebuffer(Graphics::DEFAULT->device(), _prim_framebuffer, nullptr);
		_prim_framebuffer = nullptr;

		vkDestroyFramebuffer(Graphics::DEFAULT->device(), _de_framebuffer, nullptr);
		_de_framebuffer = nullptr;

	}

	util::Result<void, PrevPass::Error> PrevPass::_create_sync_objects() {
		if (auto err = Semaphore::create().move_or(_semaphore)) {
			return Error(ErrorType::VULKAN, "Could not create seamphore for the prev pass", VkError(err.value()));
		}

		if (auto err = Fence::create().move_or(_fence)) {
			return Error(ErrorType::VULKAN, "Could not create fence for the prev pass", VkError(err.value()));
		}

		return {};
	}

	void PrevPass::_create_command_buffers() {
		auto alloc_info = VkCommandBufferAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = Graphics::DEFAULT->command_pool();
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		util::require(vkAllocateCommandBuffers(Graphics::DEFAULT->device(), &alloc_info, &_command_buffer));
	}


	VkFormat PrevPass::_depth_format() {
		return Graphics::DEFAULT->find_supported_format(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	std::string PrevPass::_codegen_de() {
		auto source_code = util::readEnvFile("assets/shaders/preview_de.frag.cg");

		auto textures = used_textures(_scene->resource_manager().textures());

		auto meshes = cg::TemplList();
		for (auto &m : _meshes) {
			meshes.push_back(m.base()->cg_templobj());
		}

		auto materials = cg::TemplList();
		for (auto &material : _materials) {
			if (!material) continue;
			materials.push_back(material_templobj(material->id(), _scene->resource_manager().materials()));
		}

		auto args = cg::TemplObj{
			{"global_declarations", GlobalPrevPassUniform::declaration_content},
			{"node_declarations", PrevPassNode::VImpl::declaration},
			{"meshes", meshes},
			{"materials", materials},
			{"texture_count", cg::TemplInt(textures.size())}
		};
		source_code = cg::TemplGen::codegen(source_code, args, "preview_de.frag.cg").value();

		log_info() << "Prev pass de code: " << util::add_strnum(source_code) << std::endl;

		return source_code;
	}
}

template<>
	const char *vulkan::PrevPass::Error::type_str(vulkan::PrevPass::ErrorType t) {
		switch (t) {
			case vulkan::PrevPass::ErrorType::VULKAN:
				return "PrevPass.VULKAN";
			case vulkan::PrevPass::ErrorType::RESOURCE:
				return "PrevPass.RESOURCE";
			case vulkan::PrevPass::ErrorType::MISC:
				return "PrevPass.MISC";
		}
	}

std::ostream &operator<<(std::ostream &os, vulkan::PrevPass::ErrorType err) {
	return os << vulkan::PrevPass::Error::type_str(err);
}

