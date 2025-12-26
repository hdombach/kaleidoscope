#include "InstancedPass.hpp"
#include "InstancedPassMesh.hpp"
#include "codegen/TemplGen.hpp"
#include "util/file.hpp"
#include "util/log.hpp"
#include "util/Util.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Error.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/Vertex.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/prev_pass/InstancedPassNode.hpp"

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

namespace vulkan {
	using MeshObserver = InstancedPass::MeshObserver;
	using NodeObserver = InstancedPass::NodeObserver;

	MeshObserver::MeshObserver(InstancedPass &instanced_pass):
		_instanced_pass(&instanced_pass)
	{ }

	void MeshObserver::obs_create(uint32_t id) { _instanced_pass->mesh_create(id); }
	void MeshObserver::obs_update(uint32_t id) { _instanced_pass->mesh_update(id); }
	void MeshObserver::obs_remove(uint32_t id) { _instanced_pass->mesh_remove(id); }

	NodeObserver::NodeObserver(InstancedPass &instanced_pass):
		_instanced_pass(&instanced_pass)
	{}

	void NodeObserver::obs_create(uint32_t id) { _instanced_pass->node_create(id); }
	void NodeObserver::obs_update(uint32_t id) { _instanced_pass->node_update(id); }
	void NodeObserver::obs_remove(uint32_t id) { _instanced_pass->node_remove(id); }

	util::Result<InstancedPass::Ptr, Error> InstancedPass::create(
		VkExtent2D size,
		Scene &scene
	) {
		auto p = Ptr(new InstancedPass());

		p->_size = size;
		p->_scene = &scene;

		p->_descriptor_pool = DescriptorPool::create();

		p->_node_observer = NodeObserver(*p);
		p->_mesh_observer = MeshObserver(*p);

		if (auto err = Fence::create().move_or(p->_fence)) {
			return Error(ErrorType::VULKAN, "Could not create fence", VkError(err.value()));
		}

		if (auto err = Semaphore::create().move_or(p->_semaphore)) {
			return Error(ErrorType::VULKAN, "Could not create semaphore", VkError(err.value()));
		}

		if (auto err = p->_create_command_buffer().move_or(p->_command_buffer)) {
			return Error(ErrorType::MISC, "Could not create command buffer", err.value());
		}

		if (auto err = p->_create_images().move_or()) {
			return Error(ErrorType::MISC, "Could not create images", err.value());
		}

		if (auto err = p->_create_uniform().move_or()) {
			return Error(ErrorType::MISC, "Could not create uniform", err.value());
		}

		if (auto err = p->_create_descriptor_set().move_or()) {
			return Error(ErrorType::MISC, "Could not create the descriptor set", err.value());
		}

		if (auto err = p->_create_render_pass().move_or(p->_render_pass)) {
			return Error(ErrorType::MISC, "Could not create render pass", err.value());
		}

		if (auto err = p->_create_pipeline(p->_pipeline, p->_pipeline_layout).move_or()) {
			return Error(ErrorType::MISC, "Could not create pipeline", err.value());
		}

		if (auto err = p->_create_framebuffers().move_or(p->_framebuffer)) {
			return Error(ErrorType::MISC, "Could not create framebuffers", err.value());
		}

		return std::move(p);
	}

	InstancedPass::InstancedPass(InstancedPass &&other) {
		_meshes = std::move(other._meshes);
		_mesh_observer = std::move(other._mesh_observer);
		_node_observer = std::move(other._node_observer);

		_scene = util::move_ptr(_scene);
		_render_pass = util::move_ptr(other._render_pass);
		_pipeline = util::move_ptr(other._pipeline);
		_pipeline_layout = util::move_ptr(other._pipeline_layout);
		_descriptor_pool = std::move(other._descriptor_pool);
		_shared_descriptor_set = std::move(other._shared_descriptor_set);
		_shared_descriptor_set_layout = std::move(other._shared_descriptor_set_layout);
		_mesh_descriptor_set_layout = std::move(other._mesh_descriptor_set_layout);
		_fence = std::move(other._fence);
		_semaphore = std::move(other._semaphore);
		_command_buffer = util::move_ptr(other._command_buffer);
		_framebuffer = util::move_ptr(other._framebuffer);
		_size = other._size;
		_depth_image = std::move(other._depth_image);
		_material_image = std::move(other._material_image);
		_result_image = std::move(other._result_image);
		_prim_uniform = std::move(other._prim_uniform);
		_imgui_descriptor_set = std::move(other._imgui_descriptor_set);
	}

	InstancedPass &InstancedPass::operator=(InstancedPass &&other) {
		_meshes = std::move(other._meshes);
		_mesh_observer = std::move(other._mesh_observer);
		_node_observer = std::move(other._node_observer);

		_scene = util::move_ptr(_scene);
		_render_pass = util::move_ptr(other._render_pass);
		_pipeline = util::move_ptr(other._pipeline);
		_pipeline_layout = util::move_ptr(other._pipeline_layout);
		_descriptor_pool = std::move(other._descriptor_pool);
		_shared_descriptor_set = std::move(other._shared_descriptor_set);
		_shared_descriptor_set_layout = std::move(other._shared_descriptor_set_layout);
		_mesh_descriptor_set_layout = std::move(other._mesh_descriptor_set_layout);
		_fence = std::move(other._fence);
		_semaphore = std::move(other._semaphore);
		_command_buffer = util::move_ptr(other._command_buffer);
		_framebuffer = util::move_ptr(other._framebuffer);
		_size = other._size;
		_depth_image = std::move(other._depth_image);
		_material_image = std::move(other._material_image);
		_result_image = std::move(other._result_image);
		_prim_uniform = std::move(other._prim_uniform);
		_imgui_descriptor_set = std::move(other._imgui_descriptor_set);

		return *this;
	}

	void InstancedPass::destroy() {
		_shared_descriptor_set.destroy();
		_destroy_render_pass();
		_destroy_pipeline();
		_destroy_command_buffer();
	}

	InstancedPass::~InstancedPass() {
		destroy();
	}

	bool InstancedPass::has_value() const {
		return _render_pass != nullptr;
	}

	InstancedPass::operator bool() const {
		return has_value();
	}

	VkSemaphore InstancedPass::render(
		VkSemaphore semaphore,
		types::Camera const &camera
	) {
		VkResult r;

		if ((r = _fence.wait()) != VK_SUCCESS) {
			log_error() << "Problem waiting on fence: " << VkError::type_str(r) << std::endl;
		}
		if ((r = _fence.reset()) != VK_SUCCESS) {
			log_error() << "Problem reseting fence: " << VkError::type_str(r) << std::endl;
		}

		util::require(vkResetCommandBuffer(_command_buffer, 0));

		auto begin_info = VkCommandBufferBeginInfo{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;
		util::require(vkBeginCommandBuffer(_command_buffer, &begin_info));

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(_size.width);
		viewport.height = static_cast<float>(_size.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(_command_buffer, 0, 1, &viewport);

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = _size;
		vkCmdSetScissor(_command_buffer, 0, 1, &scissor);

		auto render_pass_info = VkRenderPassBeginInfo{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = _render_pass;
		render_pass_info.framebuffer = _framebuffer;
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = _size;

		auto clear_values = std::array{
			VkClearValue{0.0f, 0.0f, 0.0f, 1.0f},
			VkClearValue{{0}},
			VkClearValue{1.0f, 0}
		};

		render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		log_assert(_pipeline, "Instanced pipeline does not exist");
		vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

		_prim_uniform.set_value(GlobalPrevPassUniform::create(camera));

		for (auto &mesh : _meshes) {
			if (mesh.is_de()) continue;
			if (mesh.instance_count() == 0) continue;

			auto descriptor_sets = std::array{
				_shared_descriptor_set.descriptor_set(),
				mesh.descriptor_set().descriptor_set()
			};

			vkCmdBindDescriptorSets(
				_command_buffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				_pipeline_layout,
				0,
				descriptor_sets.size(),
				descriptor_sets.data(),
				0,
				nullptr
			);

			VkBuffer vertex_buffers[] = {mesh.vertex_buffer().buffer()};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(_command_buffer, 0, 1, vertex_buffers, offsets);
			vkCmdBindIndexBuffer(
				_command_buffer,
				mesh.index_buffer().buffer(),
				0,
				VK_INDEX_TYPE_UINT32
			);

			vkCmdDrawIndexed(
				_command_buffer,
				mesh.index_count(),
				mesh.instance_count(),
				0, 0, 0
			);
		}

		vkCmdEndRenderPass(_command_buffer);

		util::require(vkEndCommandBuffer(_command_buffer), "Problem ending command buffer: ");

		VkSemaphore finish_semaphore = _semaphore.get();

		auto wait_stages = std::array{
			VkPipelineStageFlags(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT),
			VkPipelineStageFlags(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
		};

		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pWaitDstStageMask = wait_stages.data();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &_command_buffer;
		if (semaphore) {
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &semaphore;
		}
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &finish_semaphore;

		util::require(
			vkQueueSubmit(Graphics::DEFAULT->graphics_queue(), 1, &submit_info, _fence.get()),
			"Problem submitting queue: "
		);

		return finish_semaphore;
	}

	VkRenderPass InstancedPass::render_pass() {
		return _render_pass;
	}

	VkDescriptorSet InstancedPass::imgui_descriptor_set() {
		return _imgui_descriptor_set;
	}

	DescriptorSetLayout const &InstancedPass::mesh_descriptor_set_layout() const {
		return _mesh_descriptor_set_layout;
	}

	VkImageView InstancedPass::image_view() {
		return _material_image.image_view();
	}

	MeshObserver &InstancedPass::mesh_observer() {
		return _mesh_observer;
	}

	NodeObserver &InstancedPass::node_observer() {
		return _node_observer;
	}

	DescriptorPool const &InstancedPass::descriptor_pool() const {
		return _descriptor_pool;
	}

	void InstancedPass::mesh_create(uint32_t id) {
		auto &raw_mesh = _scene->resource_manager().meshes()[id];
		log_assert(raw_mesh != nullptr, util::f("Mesh ", id, " does not exist"));
		if (auto mesh = InstancedPassMesh::create(raw_mesh.get(), *this)) {
			log_trace() << "Adding/updating instance pass mesh handler " << mesh->id() << std::endl;
			_meshes.insert(std::move(mesh.value()));
		} else {
			log_error()
				<< "Couldn't create a mesh " << id << " for the InstancedPass " << std::endl
				<< mesh.error()
				<< std::endl;
		}
	}

	void InstancedPass::mesh_update(uint32_t id) {
		mesh_create(id);
	}

	void InstancedPass::mesh_remove(uint32_t id) {
		log_trace() << "Removing instance pass mesh handler " << id << std::endl;
		_meshes[id].destroy();
	}

	void InstancedPass::node_create(uint32_t id) {
		auto &raw_node = _scene->nodes()[id];
		log_assert(raw_node != nullptr, util::f("Node ", id, " does not exist"));
		auto node = InstancedPassNode::create(*raw_node);
		log_trace() << "Adding instanced pass node handler " << node.id() << std::endl;
		_nodes.insert(std::move(node));

		if (raw_node->type() != Node::Type::Object) return;

		auto &mesh = _meshes[raw_node->mesh().id()];
		mesh.add_node(*raw_node);
		_nodes[id].registered_mesh = mesh.id();
	}

	void InstancedPass::node_update(uint32_t id) {
		log_trace() << "Updating instanced pass node handler " << id << std::endl;

		auto &node = _nodes[id];
		auto &raw_node = _scene->nodes()[id];

		if (raw_node->type() != Node::Type::Object) return;
		if (raw_node->mesh().id() == node.registered_mesh) return;

		auto &old_mesh = _meshes[node.registered_mesh];
		if (!old_mesh.has_value()) {
			log_error() << "Old mesh " << old_mesh.id()
				<< " is not known in the InstancedPass" << std::endl;
		}

		auto &new_mesh = _meshes[raw_node->mesh().id()];
		log_assert(
			new_mesh.has_value(),
			util::f("Mesh ", raw_node->mesh().id(), " is not known in the InstancedPass")
		);

		old_mesh.remove_node(id);
		new_mesh.add_node(*raw_node);

		node.registered_mesh = raw_node->mesh().id();
	}

	void InstancedPass::node_remove(uint32_t id) {
		log_trace() << "Removing instance pass node handler " << std::endl;
		auto &node = *_scene->nodes()[id];
		if (node.type() != Node::Type::Object) return;

		auto &mesh = _meshes[node.mesh().id()];
		log_assert(mesh.has_value(), util::f(
			"Node ", id, " does not have a valid mesh ", node.mesh().id()
		));

		mesh.remove_node(id);
	}

	util::Result<VkRenderPass, Error> InstancedPass::_create_render_pass() {
		auto result_attachment = VkAttachmentDescription{};
		result_attachment.format = _RESULT_IMAGE_FORMAT;
		result_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		result_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		result_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		result_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		result_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		result_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		result_attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto result_attachment_ref = VkAttachmentReference{};
		result_attachment_ref.attachment = 0;
		result_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto material_attachment = VkAttachmentDescription{};
		material_attachment.format = _MATERIAL_IMAGE_FORMAT;
		material_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		material_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		material_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		material_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		material_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		material_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		material_attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto material_attachment_ref = VkAttachmentReference{};
		material_attachment_ref.attachment = 1;
		material_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
		depth_attachment_ref.attachment = 2;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		auto color_attachment_refs = std::array{
			result_attachment_ref,
			material_attachment_ref
		};

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = color_attachment_refs.size();
		subpass.pColorAttachments = color_attachment_refs.data();
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		auto attachments = std::array{
			result_attachment,
			material_attachment,
			depth_attachment
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

		VkRenderPass render_pass;
		auto res = vkCreateRenderPass(
			Graphics::DEFAULT->device(),
			&render_pass_info,
			nullptr,
			&render_pass
		);

		if (res == VK_SUCCESS) {
			return {render_pass};
		} else {
			return Error(ErrorType::VULKAN, "Could not create renderpass", VkError(res));
		}
	}

	void InstancedPass::_destroy_render_pass() {
		if (_render_pass) {
			vkDestroyRenderPass(Graphics::DEFAULT->device(), _render_pass, nullptr);
			_render_pass = nullptr;
		}
	}

	util::Result<void, Error> InstancedPass::_create_pipeline(
		VkPipeline &pipeline,
		VkPipelineLayout &pipeline_layout
	) {
		_destroy_pipeline();

		Shader vert_shader, frag_shader;

		{
			auto start = log_start_timer();

			auto source_code = util::readEnvFile("assets/shaders/instanced.vert.cg");
			auto args = cg::TemplObj{
				{"global_declarations", GlobalPrevPassUniform::declaration_content}
			};
			if (auto err = cg::TemplGen::codegen( source_code, args, "instanced.vert.cg").move_or(source_code)) {
				return Error(
					ErrorType::MISC,
					"Problem codegenerating instanced vert shader",
					err.value()
				);
			}
			log_info() << "Instance shader took " << start << std::endl;
			log_info() << "Generated instanced.vert.cg:" << std::endl
				<< util::add_strnum(source_code) << std::endl;

			if (auto err = Shader::from_source_code(
				source_code, Shader::Type::Vertex
			).move_or(vert_shader)) {
				return Error(ErrorType::MISC, "Problem parsing instanced vert shader", err.value());
			}
		}

		{
			auto start = log_start_timer();
			auto args = cg::TemplDict{};

			auto source_code = util::readEnvFile("assets/shaders/instanced.frag.cg");
			if (auto err = cg::TemplGen::codegen(source_code, args, "instanced.frag.cg").move_or(source_code)) {
				return Error(ErrorType::MISC, "Problem parsing instanced frag shader", err.value());
			}
			log_info() << "Instanced shader took " << start << std::endl;
			log_info() << "Generated instanced.frag.cg:" << std::endl
				<< util::add_strnum(source_code) << std::endl;

			if (auto err = Shader::from_source_code(
					source_code, Shader::Type::Fragment
			).move_or(frag_shader)) {
				return Error(ErrorType::MISC, "Problem parsing instanced frag shader", err.value());
			}
		}

		auto shader_stages = std::array<VkPipelineShaderStageCreateInfo, 2>();

		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = vert_shader.shader_module();
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = frag_shader.shader_module();
		shader_stages[1].pName = "main";

		auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{};

		auto binding_description = Vertex::getBindingDescription();
		auto attribute_descriptions = Vertex::getAttributeDescriptions();

		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = attribute_descriptions.size();
		vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

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

		auto result_blend_attachment = VkPipelineColorBlendAttachmentState{};
		result_blend_attachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		result_blend_attachment.blendEnable = VK_TRUE;
		result_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		result_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		result_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		result_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		result_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		result_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		auto material_blend_attachment = VkPipelineColorBlendAttachmentState{};
		material_blend_attachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		material_blend_attachment.blendEnable = VK_FALSE;
		material_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		material_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		material_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		material_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		material_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		material_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		auto depth_buf_blend_attachment = VkPipelineColorBlendAttachmentState{};
		depth_buf_blend_attachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		depth_buf_blend_attachment.blendEnable = VK_FALSE;
		depth_buf_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		depth_buf_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		depth_buf_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		depth_buf_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		depth_buf_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		depth_buf_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		auto color_attachments = std::array{
			result_blend_attachment,
			material_blend_attachment,
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

		auto dynamic_states = std::array{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		auto dynamic_state = VkPipelineDynamicStateCreateInfo{};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
		dynamic_state.pDynamicStates = dynamic_states.data();

		auto layouts = std::array{
			_shared_descriptor_set_layout.layout(),
			_mesh_descriptor_set_layout.layout()
		};

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.pSetLayouts = layouts.data();
		pipeline_layout_info.setLayoutCount = layouts.size();
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		auto res = vkCreatePipelineLayout(
			Graphics::DEFAULT->device(),
			&pipeline_layout_info,
			nullptr,
			&pipeline_layout
		);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create pipeline layout", VkError(res));
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
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages.data();
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pDepthStencilState = &depth_stencil;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_state;
		pipeline_info.layout = pipeline_layout;
		pipeline_info.renderPass = _render_pass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		res = vkCreateGraphicsPipelines(
			Graphics::DEFAULT->device(), 
			VK_NULL_HANDLE,
			1, 
			&pipeline_info,
			nullptr, 
			&pipeline
			);

		if (res != VK_SUCCESS) {
			vkDestroyPipelineLayout(Graphics::DEFAULT->device(), pipeline_layout, nullptr);
			return Error(ErrorType::VULKAN, "Could not create the graphics pipeline", VkError(res));
		}

		return {};
	}

	void InstancedPass::_destroy_pipeline() {
		if (_pipeline_layout) {
			vkDestroyPipelineLayout(Graphics::DEFAULT->device(), _pipeline_layout, nullptr);
			_pipeline_layout = nullptr;
		}

		if (_pipeline) {
			vkDestroyPipeline(Graphics::DEFAULT->device(), _pipeline, nullptr);
			_pipeline = nullptr;
		}
	}

	util::Result<void, Error> InstancedPass::_create_images() {
		if (auto err = Image::create(
			_size,
			_depth_format(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		).move_or(_depth_image)) {
			return Error(ErrorType::VULKAN, "Could not create depth image", err.value());
		}

		if (auto err = Image::create(
			_size,
			_MATERIAL_IMAGE_FORMAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_STORAGE_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT
		).move_or(_material_image)) {
			return Error(ErrorType::VULKAN, "Could not create material image", err.value());
		}

		if (auto err = Image::create(
			_size,
			_RESULT_IMAGE_FORMAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_STORAGE_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT
		).move_or(_result_image)) {
			return Error(ErrorType::VULKAN, "Could not create result image", err.value());
		}

		_imgui_descriptor_set = ImGui_ImplVulkan_AddTexture(
			*Graphics::DEFAULT->main_texture_sampler(),
			_result_image.image_view(),
			VK_IMAGE_LAYOUT_GENERAL
		);

		return {};
	}

	util::Result<VkCommandBuffer, Error> InstancedPass::_create_command_buffer() {
		auto alloc_info = VkCommandBufferAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandPool = Graphics::DEFAULT->command_pool();
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		auto res = vkAllocateCommandBuffers(
			Graphics::DEFAULT->device(),
			&alloc_info,
			&command_buffer
		);
		if (res == VK_SUCCESS) {
			return command_buffer;
		} else {
			return Error(ErrorType::VULKAN, "Could not allocate the command buffer", VkError(res));
		}
	}

	void InstancedPass::_destroy_command_buffer() { }

	util::Result<VkFramebuffer, Error> InstancedPass::_create_framebuffers() {
		auto attachments = std::array{
			_result_image.image_view(),
			_material_image.image_view(),
			_depth_image.image_view()
		};

		auto framebuffer_info = VkFramebufferCreateInfo{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = _render_pass;
		framebuffer_info.attachmentCount = attachments.size();
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = _size.width;
		framebuffer_info.height = _size.height;
		framebuffer_info.layers = 1;

		VkFramebuffer framebuffer;
		auto res = vkCreateFramebuffer(
			Graphics::DEFAULT->device(),
			&framebuffer_info,
			nullptr,
			&framebuffer
		);

		if (res == VK_SUCCESS) {
			return framebuffer;
		} else {
			return Error(ErrorType::VULKAN, "Could not create the framebuffer", VkError(res));
		}
	}

	void InstancedPass::_destroy_framebuffers() {
		vkDestroyFramebuffer(Graphics::DEFAULT->device(), _framebuffer, nullptr);
		_framebuffer = nullptr;
	}

	VkFormat InstancedPass::_depth_format() {
		return Graphics::DEFAULT->find_supported_format(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	util::Result<void, Error> InstancedPass::_create_descriptor_set() {
		//Shared descriptor set layout
		{
			auto bindings = std::vector<VkDescriptorSetLayoutBinding>();
			bindings.push_back(descriptor_layout_image(VK_SHADER_STAGE_FRAGMENT_BIT));
			bindings.push_back(descriptor_layout_image(VK_SHADER_STAGE_FRAGMENT_BIT));
			bindings.push_back(descriptor_layout_image(VK_SHADER_STAGE_FRAGMENT_BIT));
			bindings.push_back(descriptor_layout_uniform(VK_SHADER_STAGE_VERTEX_BIT));

			if (auto err = DescriptorSetLayout::create(bindings).move_or(_shared_descriptor_set_layout)) {
				return Error(ErrorType::SHADER_RESOURCE, "Could not create descriptor set layout", err.value());
			}

			auto builder = _shared_descriptor_set_layout.builder();
			if (auto err = builder.add_image(_result_image.image_view(), VK_IMAGE_LAYOUT_GENERAL).move_or()) {
				return Error(ErrorType::SHADER_RESOURCE, "Could not add result image", err.value());
			}
			if (auto err = builder.add_image(_material_image.image_view(), VK_IMAGE_LAYOUT_GENERAL).move_or()) {
				return Error(ErrorType::SHADER_RESOURCE, "Could not add material image", err.value());
			}
			if (auto err = builder.add_image(_depth_image.image_view(), VK_IMAGE_LAYOUT_GENERAL).move_or()) {
				return Error(ErrorType::SHADER_RESOURCE, "Could not add depth image", err.value());
			}
			if (auto err = builder.add_uniform(_prim_uniform).move_or()) {
				return Error(ErrorType::SHADER_RESOURCE, "Could not add prim uniform", err.value());
			}

			if (auto err = DescriptorSets::create(builder, _descriptor_pool).move_or(_shared_descriptor_set)) {
				return Error(ErrorType::SHADER_RESOURCE, "Could not create descriptor sets", err.value());
			}
		}

		//Mesh descriptor set layout
		{
			auto bindings = std::vector<VkDescriptorSetLayoutBinding>();
			//For node buffer
			bindings.push_back(descriptor_layout_storage_buffer(VK_SHADER_STAGE_FRAGMENT_BIT));

			if (auto err = DescriptorSetLayout::create(bindings).move_or(_mesh_descriptor_set_layout)) {
				return Error(ErrorType::SHADER_RESOURCE, "Could not create mesh descriptor set layout", err.value());
			}
		}

		return {};
	}

	util::Result<void, Error> InstancedPass::_create_uniform() {
		if (auto err = MappedPrevPassUniform::create().move_or(_prim_uniform)) {
			return Error(ErrorType::SHADER_RESOURCE, "Could not create uniform", err.value());
		}

		return {};
	}
}
