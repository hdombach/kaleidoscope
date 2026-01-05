#include "InstancedPass.hpp"
#include "InstancedPassMesh.hpp"
#include "codegen/TemplGen.hpp"
#include "util/file.hpp"
#include "util/log.hpp"
#include "util/Util.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Error.hpp"
#include "vulkan/FrameAttachment.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/prev_pass/InstancedPassNode.hpp"
#include "vulkan/prev_pass/InstancedPassMesh.hpp"

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

		if (auto err = p->_create_render_pass().move_or(p->_render_pass)) {
			return Error(ErrorType::MISC, "Could not create render pass", err.value());
		}

		if (auto err = p->_create_pipeline(p->_pipeline).move_or()) {
			return Error(ErrorType::MISC, "Could not create pipeline", err.value());
		}

		if (auto err = p->_create_descriptor_set().move_or()) {
			return Error(ErrorType::MISC, "Could not create the descriptor set", err.value());
		}

		return std::move(p);
	}

	InstancedPass::InstancedPass(InstancedPass &&other) {
		_meshes = std::move(other._meshes);
		_mesh_observer = std::move(other._mesh_observer);
		_node_observer = std::move(other._node_observer);

		_scene = util::move_ptr(_scene);
		_render_pass = std::move(other._render_pass);
		_pipeline = std::move(other._pipeline);
		_descriptor_pool = std::move(other._descriptor_pool);
		_shared_descriptor_set = std::move(other._shared_descriptor_set);
		_fence = std::move(other._fence);
		_semaphore = std::move(other._semaphore);
		_command_buffer = util::move_ptr(other._command_buffer);
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
		_render_pass = std::move(other._render_pass);
		_pipeline = std::move(other._pipeline);
		_descriptor_pool = std::move(other._descriptor_pool);
		_shared_descriptor_set = std::move(other._shared_descriptor_set);
		_fence = std::move(other._fence);
		_semaphore = std::move(other._semaphore);
		_command_buffer = util::move_ptr(other._command_buffer);
		_size = other._size;
		_depth_image = std::move(other._depth_image);
		_material_image = std::move(other._material_image);
		_result_image = std::move(other._result_image);
		_prim_uniform = std::move(other._prim_uniform);
		_imgui_descriptor_set = std::move(other._imgui_descriptor_set);

		return *this;
	}

	void InstancedPass::destroy() {
		_meshes.clear();
		_nodes.clear();
		_pipeline.destroy();
		_shared_descriptor_set.destroy();
		_descriptor_pool.destroy();
		_render_pass.destroy();
		_pipeline.destroy();
		_fence.destroy();
		_semaphore.destroy();
		_depth_image.destroy();
		_material_image.destroy();
		_result_image.destroy();
		_prim_uniform.destroy();
	}

	InstancedPass::~InstancedPass() {
		destroy();
	}

	bool InstancedPass::has_value() const {
		return _render_pass.has_value();
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
		render_pass_info.renderPass = _render_pass.render_pass();
		render_pass_info.framebuffer = _render_pass.framebuffer();
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = _size;

		auto clear_values = _render_pass.clear_values();

		render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_info.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		log_assert(_pipeline, "Instanced pipeline does not exist");
		vkCmdBindPipeline(_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeline());

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
				_pipeline.pipeline_layout(),
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

	RenderPass const &InstancedPass::render_pass() const {
		return _render_pass;
	}

	VkDescriptorSet InstancedPass::imgui_descriptor_set() {
		return _imgui_descriptor_set;
	}

	DescriptorSetLayout const &InstancedPass::mesh_descriptor_set_layout() const {
		return _pipeline.layouts()[1];
	}

	Pipeline::Attachments const &InstancedPass::pipeline_attachments() const {
		return _pipeline.attachments();
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

		auto &new_mesh = _meshes[raw_node->mesh().id()];
		log_assert(
			new_mesh.has_value(),
			util::f("Mesh ", raw_node->mesh().id(), " is not known in the InstancedPass")
		);

		if (raw_node->mesh().id() != node.registered_mesh) {
			auto &old_mesh = _meshes[node.registered_mesh];
			if (!old_mesh.has_value()) {
				log_error() << "Old mesh " << old_mesh.id()
					<< " is not known in the InstancedPass" << std::endl;
			}

			old_mesh.remove_node(id);
			new_mesh.add_node(*raw_node);

			node.registered_mesh = raw_node->mesh().id();
		}

		if (auto err = new_mesh.update_node(*raw_node).move_or()) {
			log_error() << "Could not update node.\n" << err.value();
		}
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

	util::Result<RenderPass, Error> InstancedPass::_create_render_pass() {
		auto frame_attachments = std::vector{
			FrameAttachment::create(_result_image),
			FrameAttachment::create(_material_image),
			FrameAttachment::create(_depth_image).set_depth()
		};

		RenderPass render_pass;

		if (auto err = RenderPass::create(std::move(frame_attachments)).move_or(render_pass)) {
			return Error(ErrorType::MISC, "Could not create instanced pass render pass", err.value());
		}

		return render_pass;
	}

	util::Result<void, Error> InstancedPass::_create_pipeline(
		Pipeline &pipeline
	) {
		_pipeline.destroy();

		Shader vert_shader, frag_shader;

		{
			auto start = log_start_timer();

			auto source_code = util::readEnvFile("assets/shaders/instanced.vert.cg");
			auto args = cg::TemplObj{
				{"global_declarations", GlobalPrevPassUniform::declaration_content},
				{"node_declaration", InstancedPassMesh::NodeVImpl::declaration}
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

		auto desc_attributes = Pipeline::Attachments{
			{
				DescAttachment::create_image(VK_SHADER_STAGE_FRAGMENT_BIT),
				DescAttachment::create_image(VK_SHADER_STAGE_FRAGMENT_BIT),
				DescAttachment::create_image(VK_SHADER_STAGE_FRAGMENT_BIT),
				DescAttachment::create_uniform(VK_SHADER_STAGE_VERTEX_BIT),
			},
			{
				DescAttachment::create_storage_buffer(VK_SHADER_STAGE_VERTEX_BIT),
			}
		};

		if (auto err = Pipeline::create_graphics(
				vert_shader,
				frag_shader,
				_render_pass,
				desc_attributes
		).move_or(_pipeline)) {
			return Error(ErrorType::MISC, "Could not create instanced pass pipeline", err.value());
		}

		return {};
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

		Graphics::DEFAULT->transition_image_layout(
			_material_image.image(),
			_RESULT_IMAGE_FORMAT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			1
		);

		if (auto err = Image::create(
			_size,
			_RESULT_IMAGE_FORMAT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			| VK_IMAGE_USAGE_STORAGE_BIT
			| VK_IMAGE_USAGE_SAMPLED_BIT
		).move_or(_result_image)) {
			return Error(ErrorType::VULKAN, "Could not create result image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_result_image.image(),
			_RESULT_IMAGE_FORMAT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			1
		);

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

	VkFormat InstancedPass::_depth_format() {
		return Graphics::DEFAULT->find_supported_format(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	util::Result<void, Error> InstancedPass::_create_descriptor_set() {
		auto attachments = _pipeline.attachments();

		log_assert(attachments.size() >= 2, "Instanced pass pipeline must be initialized");
		log_assert(
			attachments[0].size() >= 4,
			"Shared descriptor set layout in the instanced pass must have enough elements"
		);

		attachments[0][0].add_image(_result_image);
		attachments[0][1].add_image(_material_image);
		attachments[0][2].add_image(_depth_image);
		attachments[0][3].add_uniform(_prim_uniform);

		if (auto err = DescriptorSets::create(
				attachments[0],
				_pipeline.layouts()[0],
				_descriptor_pool
		).move_or(_shared_descriptor_set)) {
			return Error(ErrorType::MISC, "Could not create InstacedPass descriptor set", err.value());
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
