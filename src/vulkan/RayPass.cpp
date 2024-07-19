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
		result->_vertex_buffer = nullptr;
		result->_vertex_buffer_memory = nullptr;
		result->_index_buffer = nullptr;
		result->_index_buffer_memory = nullptr;

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
		_destroy_mesh_buffers();
		_destroy_node_buffers();
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

		_vertex_buffer = other._vertex_buffer;
		other._vertex_buffer = nullptr;
		_vertex_buffer_memory = other._vertex_buffer_memory;
		other._vertex_buffer_memory = nullptr;
		_vertex_buffer_range = other._vertex_buffer_range;

		_index_buffer = other._index_buffer;
		other._index_buffer = nullptr;
		_index_buffer_memory = other._index_buffer_memory;
		other._index_buffer_memory = nullptr;
		_index_buffer_range = other._index_buffer_range;

		_mesh_buffer = other._mesh_buffer;
		other._mesh_buffer = nullptr;
		_mesh_buffer_memory = other._mesh_buffer_memory;
		other._mesh_buffer_memory = nullptr;
		_mesh_buffer_range = other._mesh_buffer_range;

		_node_buffer = other._node_buffer;
		other._node_buffer = nullptr;
		_node_buffer_memory = other._node_buffer_memory;
		other._node_buffer_memory = nullptr;
		_node_buffer_range = other._node_buffer_range;

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
			_create_pipeline();
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
		_destroy_mesh_buffers();
		_create_mesh_buffers();
		_create_descriptor_sets();
	}

	void RayPass::mesh_update(uint32_t id) {}

	void RayPass::mesh_remove(uint32_t id) {}

	void RayPass::node_create(uint32_t id) {
		_nodes.push_back(RayPassNode(_scene->get_node(id), this));
		_destroy_node_buffers();
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
					_vertex_buffer, 
					_vertex_buffer_range));

		descriptor_templates.push_back(DescriptorSetTemplate::create_storage_buffer(
					3,
					VK_SHADER_STAGE_COMPUTE_BIT, 
					_index_buffer, 
					_index_buffer_range));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				1,
				_descriptor_pool);

		TRY(descriptor_sets);
		_descriptor_set = std::move(descriptor_sets.value());

		return {};
	}

	util::Result<void, KError> RayPass::_create_pipeline() {
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
		_vertex_buffer_range = vertices.size() * sizeof(vertices[0]);
		_index_buffer_range = indices.size() * sizeof(indices[0]);

		/* move vertex buffer to gpu */
		_create_comp_buffer(
				vertices.data(),
				_vertex_buffer,
				_vertex_buffer_memory,
				_vertex_buffer_range);
		LOG_DEBUG << "creating vertex buffer " << _vertex_buffer << std::endl;

		/* move index buffer to gpu */
		_create_comp_buffer(
				indices.data(), 
				_index_buffer, 
				_index_buffer_memory, 
				_index_buffer_range);
	}

	void RayPass::_destroy_mesh_buffers() {
		if (_vertex_buffer) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), _vertex_buffer, nullptr);
			_vertex_buffer = nullptr;
		}
		if (_vertex_buffer_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _vertex_buffer_memory, nullptr);
			_vertex_buffer_memory = nullptr;
		}
		if (_index_buffer) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), _index_buffer, nullptr);
			_index_buffer = nullptr;
		}
		if (_index_buffer_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _index_buffer_memory, nullptr);
			_index_buffer_memory = nullptr;
		}
		if (_mesh_buffer) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), _mesh_buffer, nullptr);
			_mesh_buffer = nullptr;
		}
		if (_mesh_buffer_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _mesh_buffer_memory, nullptr);
			_mesh_buffer_memory = nullptr;
		}
	}

	void RayPass::_create_node_buffers() {
		auto nodes = std::vector<RayPassNode::VImpl>();
		for (auto &node : _nodes) {
			nodes.push_back(node.vimpl());
		}

		_node_buffer_range = nodes.size() * sizeof(nodes[0]);

		_create_comp_buffer(
				nodes.data(), 
				_node_buffer,
				_node_buffer_memory,
				_node_buffer_range);
	}

	void RayPass::_destroy_node_buffers() {
		if (_node_buffer) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), _node_buffer, nullptr);
			_node_buffer = nullptr;
		}
		if (_node_buffer_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _node_buffer_memory, nullptr);
			_node_buffer_memory = nullptr;
		}
	}

	void RayPass::_create_comp_buffer(
			void *data,
			VkBuffer &buffer,
			VkDeviceMemory &buffer_memory,
			VkDeviceSize range)
	{
		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		Graphics::DEFAULT->create_buffer(
				range,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staging_buffer, 
				staging_buffer_memory);

		void *mapped_data;
		vkMapMemory(
				Graphics::DEFAULT->device(),
				staging_buffer_memory,
				0,
				range,
				0,
				&mapped_data);
		memcpy(
				mapped_data,
				data,
				range);
		vkUnmapMemory(
				Graphics::DEFAULT->device(),
				staging_buffer_memory);

		Graphics::DEFAULT->create_buffer(
				range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
				buffer, 
				buffer_memory);

		Graphics::DEFAULT->copy_buffer(
				staging_buffer, 
				buffer, 
				range);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staging_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staging_buffer_memory, nullptr);
	}

}
