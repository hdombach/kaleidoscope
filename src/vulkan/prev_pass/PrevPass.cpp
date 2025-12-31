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
#include "util/PrintTools.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/FrameAttachment.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/Uniforms.hpp"
#include "vulkan/graphics.hpp"
#include "vulkan/RenderPass.hpp"
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
		static uint32_t count = 0;
		if (count < 2) {
			count++;
		} else {
			//exit(0);
		}

		if (_de_pipe_dirty_bit) {
			log_trace() << "Creating de pipeline" << std::endl;
			if (auto err = _create_de_pipeline().move_or()) {
				log_error() << err.value() << std::endl;
			}
			_de_pipe_dirty_bit = false;
		}

		if (_de_buf_dirty_bit) {
			log_trace() << "Create de buffers and de descriptor set" << std::endl;
			if (auto err = _create_de_buffers().move_or()) {
				log_error() << err.value() << std::endl;
			}
			if (auto err = _create_de_descriptor_set().move_or()) {
				log_error() << err.value() << std::endl;
			}
			_de_buf_dirty_bit = false;
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
			int i;

			auto render_pass_info = VkRenderPassBeginInfo{};
			render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_info.renderPass = _de_pipeline.render_pass().render_pass();
			render_pass_info.framebuffer = _de_pipeline.framebuffer();
			render_pass_info.renderArea.offset = {0, 0};
			render_pass_info.renderArea.extent = _size;

			i = 0;
			auto clear_values = _de_pipeline.clear_values();

			render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_info.pClearValues = clear_values.data();

			vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _de_pipeline.pipeline());

			auto descriptor_sets = std::array<VkDescriptorSet, 2>{
				_de_shared_descriptor_set.descriptor_set(),
				_de_descriptor_set.descriptor_set(),
			};

			vkCmdBindDescriptorSets(
					command_buffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					_de_pipeline.pipeline_layout(),
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
		_de_descriptor_set.destroy();

		auto textures = used_textures(_scene->resource_manager().textures());

		auto attachments = _de_pipeline.attachments()[0];

		attachments[0].add_uniform(_prim_uniform);

		if (auto err = DescriptorSets::create(
				attachments,
				_de_pipeline.layouts()[0],
				descriptor_pool()
		).move_or(_de_shared_descriptor_set)) {
			return Error(ErrorType::RESOURCE, "Could not create de shared descriptor set", err.value());
		}

		attachments = _de_pipeline.attachments()[1];

		attachments[0].add_image(_depth_buf_image);
		attachments[1].add_image(_node_image);
		attachments[2].add_buffer(_de_node_buffer);
		attachments[3].add_buffer(_de_material_buffer);
		if (textures.size() > 0) {
			attachments[4].add_images(textures);
		}

		attachments[1].set_sampler(Graphics::DEFAULT->near_texture_sampler());

		if (auto err = DescriptorSets::create(
				attachments,
				_de_pipeline.layouts()[1],
				descriptor_pool()
		) .move_or(_de_descriptor_set)) {
			return Error(ErrorType::RESOURCE, "Could not create de descriptor set", err.value());
		}

		return {};
	}

	util::Result<void, PrevPass::Error> PrevPass::_create_de_render_pass() {
		_destroy_de_render_pass();

		auto attachments = std::vector{
			FrameAttachment::create(_color_image),
			FrameAttachment::create(_de_node_image)
		};

		if (auto err = RenderPass::create(std::move(attachments)).move_or(_de_render_pass)) {
			return Error(
				ErrorType::MISC,
				"Could not create de render pass",
				err.value()
			);
		}

		return {};
	}

	void PrevPass::_destroy_de_descriptor_set() {
		_de_descriptor_set.destroy();
		_de_shared_descriptor_set.destroy();
	}

	void PrevPass::_destroy_de_render_pass() {
		_de_render_pass.destroy();
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

		auto textures = used_textures(_scene->resource_manager().textures());

		_de_desc_attachments.resize(2);

		_de_desc_attachments[0].push_back(DescAttachment::create_uniform(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));

		_de_desc_attachments[1].push_back(DescAttachment::create_image(VK_SHADER_STAGE_FRAGMENT_BIT));
		_de_desc_attachments[1].push_back(DescAttachment::create_image(VK_SHADER_STAGE_FRAGMENT_BIT));
		_de_desc_attachments[1].push_back(DescAttachment::create_storage_buffer(VK_SHADER_STAGE_FRAGMENT_BIT));
		_de_desc_attachments[1].push_back(DescAttachment::create_storage_buffer(VK_SHADER_STAGE_FRAGMENT_BIT));
		if (textures.size() > 0) {
			_de_desc_attachments[1].push_back(DescAttachment::create_images(VK_SHADER_STAGE_FRAGMENT_BIT, textures.size()));

			_de_desc_attachments[1][4].set_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		if (auto err = Pipeline::create_graphics(
			vert_shader.value(),
			frag_shader.value(),
			_de_render_pass,
			_de_desc_attachments
		).move_or(_de_pipeline)) {
			return Error(ErrorType::MISC, "Could not create graphics pipeline", err.value());
		}

		return {};
	}

	void PrevPass::_destroy_de_pipeline() {
		_de_pipeline.destroy();
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

		_de_frame_attachments.resize(2);
		_de_frame_attachments[0] = FrameAttachment::create(_color_image);
		_de_frame_attachments[1] = FrameAttachment::create(_de_node_image);


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

		return {};
	}

	void PrevPass::_destroy_framebuffers() {
		vkDestroyFramebuffer(Graphics::DEFAULT->device(), _prim_framebuffer, nullptr);
		_prim_framebuffer = nullptr;
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

