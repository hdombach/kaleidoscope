#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#include <imgui_impl_vulkan.h>

#include "RayPass.hpp"
#include "RayPassMesh.hpp"
#include "RayPassNode.hpp"
#include "Shader.hpp"
#include "graphics.hpp"
#include "Scene.hpp"
#include "../types/Node.hpp"
#include "../util/file.hpp"
#include "../util/Util.hpp"

namespace vulkan {
	RayPass::MeshObserver::MeshObserver(RayPass &ray_pass):
		_ray_pass(&ray_pass)
	{}

	void RayPass::MeshObserver::obs_create(uint32_t id) {
		_ray_pass->mesh_create(id);
	}

	void RayPass::MeshObserver::obs_update(uint32_t id) {
		_ray_pass->mesh_update(id);
	}

	void RayPass::MeshObserver::obs_remove(uint32_t id) {
		_ray_pass->mesh_remove(id);
	}

	RayPass::NodeObserver::NodeObserver(RayPass &ray_pass):
		_ray_pass(&ray_pass)
	{}

	void RayPass::NodeObserver::obs_create(uint32_t id) {
		_ray_pass->node_create(id);
	}

	void RayPass::NodeObserver::obs_update(uint32_t id) {
		_ray_pass->node_update(id);
	}

	void RayPass::NodeObserver::obs_remove(uint32_t id) {
		_ray_pass->node_remove(id);
	}

	util::Result<RayPass::Ptr, KError> RayPass::create(
			Scene &scene,
			VkExtent2D size)
	{
		auto result = std::unique_ptr<RayPass>(new RayPass());

		result->_scene = &scene;
		result->_size = size;

		result->_descriptor_pool = DescriptorPool::create();

		result->_mesh_observer = MeshObserver(*result);
		result->_node_observer = NodeObserver(*result);

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

	void RayPass::destroy() {
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

	RayPass::~RayPass() {
		destroy();
	}

	RayPass::RayPass(RayPass &&other):
		_descriptor_set(std::move(other._descriptor_set))
	{
		_size = other._size;
		_result_image = std::move(other._result_image);
		_result_image_view = std::move(other._result_image_view);
		_pass_fence = std::move(other._pass_fence);
		_pass_semaphore = std::move(other._pass_semaphore);
		_descriptor_pool = std::move(other._descriptor_pool);
		_descriptor_set = std::move(other._descriptor_set);

		_imgui_descriptor_set = other._imgui_descriptor_set;
		other._imgui_descriptor_set = nullptr;
		
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_command_buffer = other._command_buffer;
		other._command_buffer = nullptr;

		_mapped_uniform = std::move(other._mapped_uniform);

		_vertex_buffer = std::move(other._vertex_buffer);

		_index_buffer = std::move(other._index_buffer);

		_mesh_buffer = std::move(other._mesh_buffer);

		_node_buffer = std::move(other._node_buffer);

		_meshes = std::move(other._meshes);

		_scene = other._scene;
	}

	RayPass& RayPass::operator=(RayPass&& other) {
		this->~RayPass();

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

	RayPass::RayPass():
	_pipeline_layout(nullptr),
	_pipeline(nullptr)
	{ }

	VkDescriptorSet RayPass::get_descriptor_set() {
		return _imgui_descriptor_set;
	}
	ImageView const &RayPass::image_view() {
		return _result_image_view;
	}

	void RayPass::submit(Node &node) {
		if (_descriptor_set.is_cleared()) {
			_create_descriptor_sets();
		}
		if (!_pipeline) {
			auto res = _create_pipeline();
			if (!res) {
				LOG_ERROR << "problem creating pipeline: " << res.error().desc() << " " << res.error().content() << std::endl;
				return;
			}
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

	MappedComputeUniform &RayPass::current_uniform_buffer() {
		return _mapped_uniform;
	}

	void RayPass::mesh_create(uint32_t id) {
		_meshes.push_back(RayPassMesh(_scene->resource_manager().get_mesh(id), this));
		_create_mesh_buffers();
		_create_descriptor_sets();
	}

	void RayPass::mesh_update(uint32_t id) {}

	void RayPass::mesh_remove(uint32_t id) {}

	void RayPass::node_create(uint32_t id) {
		_nodes.push_back(RayPassNode(_scene->get_node(id), this));
		_create_node_buffers();
		_create_descriptor_sets();
	}

	void RayPass::node_update(uint32_t id) {}

	void RayPass::node_remove(uint32_t id) {}

	util::Result<void, KError> RayPass::_create_descriptor_sets() {
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
					_vertex_buffer));

		descriptor_templates.push_back(DescriptorSetTemplate::create_storage_buffer(
					3,
					VK_SHADER_STAGE_COMPUTE_BIT, 
					_index_buffer));

		descriptor_templates.push_back(DescriptorSetTemplate::create_storage_buffer(
					4,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_mesh_buffer));

		descriptor_templates.push_back(DescriptorSetTemplate::create_storage_buffer(
					5,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_node_buffer));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				1,
				_descriptor_pool);

		TRY(descriptor_sets);
		_descriptor_set = std::move(descriptor_sets.value());

		return {};
	}

	util::Result<void, KError> RayPass::_create_pipeline() {
		auto compute_shader = Shader::from_source_code(_codegen(), Shader::Type::Compute);
		TRY(compute_shader);

		auto compute_shader_stage_info = VkPipelineShaderStageCreateInfo{};
		compute_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compute_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_stage_info.module = compute_shader.value().shader_module();
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

		LOG_DEBUG << "just created raypass pipeline " << _pipeline << std::endl;

		return {};
	}

	void RayPass::_create_mesh_buffers() {
		/* setup buffer on cpu */
		auto vertices = std::vector<vulkan::Vertex>();
		auto indices = std::vector<uint32_t>();
		auto meshes = std::vector<RayPassMesh::VImpl>();
		for (auto &mesh : _meshes) {
			auto vertex_chunk = std::vector<vulkan::Vertex>();
			auto index_chunk = std::vector<uint32_t>();
			auto unique_vertices = std::unordered_map<vulkan::Vertex, uint32_t>();
			for (auto &vertex : *mesh.base_mesh()) {
				if (unique_vertices.count(vertex) == 0) {
					unique_vertices[vertex] = static_cast<uint32_t>(vertex_chunk.size());
					vertex_chunk.push_back(vertex);
				}
				index_chunk.push_back(unique_vertices[vertex]);
			}

			uint32_t vertex_start = vertices.size();
			uint32_t index_start = indices.size();
			vertices.insert(vertices.end(), vertex_chunk.begin(), vertex_chunk.end());
			indices.insert(indices.end(), index_chunk.begin(), index_chunk.end());
			mesh.update_offsets(vertex_start, vertex_chunk.size(), index_start, index_chunk.size());

			meshes.push_back(mesh.vimpl());
		}
		auto vertex_buffer = StaticBuffer::create(vertices);
		//TODO: error handling
		_vertex_buffer = std::move(vertex_buffer.value());

		auto index_buffer = StaticBuffer::create(indices);
		//TODO: error handling
		_index_buffer = std::move(index_buffer.value());

		auto mesh_buffer = StaticBuffer::create(meshes);
		//TODO: error handling
		_mesh_buffer = std::move(mesh_buffer.value());
	}

	void RayPass::_create_node_buffers() {
		auto nodes = std::vector<RayPassNode::VImpl>();
		for (auto &node : _nodes) {
			nodes.push_back(node.vimpl());
		}

		auto node_buffer = StaticBuffer::create(nodes);
		//TODO: error handling
		_node_buffer = std::move(node_buffer.value());
	}

	std::string RayPass::_codegen() {
		auto source = util::readEnvFile("assets/default_shader.comp");

		util::replace_substr(source, "/*MESH_DECL*/\n", RayPassMesh::VImpl::declaration());
		util::replace_substr(source, "/*NODE_DECL*/\n", RayPassNode::VImpl::declaration());

		LOG_DEBUG << "codegen: " << source << std::endl;

		return source;
	}
}