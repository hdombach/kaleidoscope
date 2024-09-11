#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
#include <imgui_impl_vulkan.h>

#include "RayPass.hpp"
#include "RayPassMaterial.hpp"
#include "RayPassMesh.hpp"
#include "RayPassNode.hpp"
#include "Shader.hpp"
#include "graphics.hpp"
#include "Scene.hpp"
#include "Vertex.hpp"
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

	RayPass::MaterialObserver::MaterialObserver(RayPass &ray_pass):
		_ray_pass(&ray_pass)
	{}

	void RayPass::MaterialObserver::obs_create(uint32_t id) {
		_ray_pass->material_create(id);
	}

	void RayPass::MaterialObserver::obs_update(uint32_t id) {
		_ray_pass->material_update(id);
	}

	void RayPass::MaterialObserver::obs_remove(uint32_t id) {
		_ray_pass->material_remove(id);
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
		result->_material_observer = MaterialObserver(*result);
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

		result->_vertex_dirty_bit = true;
		result->_node_dirty_bit = true;
		result->_material_dirty_bit = true;

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

		_bvnode_buffer = std::move(other._bvnode_buffer);

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

	VkDescriptorSet RayPass::imgui_descriptor_set() {
		return _imgui_descriptor_set;
	}
	ImageView const &RayPass::image_view() {
		return _result_image_view;
	}

	void RayPass::submit(Node &node) {
		_update_buffers();
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

	size_t RayPass::max_material_range() const {
		size_t res = 0;

		for (auto &material : _materials) {
			if (auto m = material.get()) {
				size_t r = m->resources().range();
				if (r > res) {
					res = r;
				}
			}
		}

		return res;
	}

	std::vector<VkImageView> RayPass::used_textures() const {
		auto result = std::vector<VkImageView>();

		for (auto &node : _nodes) {
			for (auto &resource : node.get().material().resources()) {
				if (auto image = resource.as_image()) {
					auto v = image.value().value();
					if (std::find(result.begin(), result.end(), v) == std::end(result)) {
						result.push_back(v);
					}
				}
			}
		}

		return result;
	}


	void RayPass::mesh_create(uint32_t id) {
		_meshes.push_back(RayPassMesh(_scene->resource_manager().get_mesh(id), this));
		_vertex_dirty_bit = true;
	}

	void RayPass::mesh_update(uint32_t id) {
		_vertex_dirty_bit = true;
	}

	void RayPass::mesh_remove(uint32_t id) {}

	void RayPass::material_create(uint32_t id) {
		while (id + 1 > _materials.size()) {
			_materials.push_back(RayPassMaterial());
		}
		_materials[id] = RayPassMaterial::create(
				_scene->resource_manager().get_material(id),
				this);
		_material_dirty_bit = true;
	}

	void RayPass::material_update(uint32_t id) {
		_materials[id] = RayPassMaterial::create(
				_scene->resource_manager().get_material(id),
				this);
		_material_dirty_bit = true;
	}

	void RayPass::material_remove(uint32_t id) {
	}

	void RayPass::node_create(uint32_t id) {
		while (id + 1 > _nodes.size()) {
			_nodes.push_back(RayPassNode());
		}
		_nodes[id] = RayPassNode::create(_scene->get_node(id), this);
		_node_dirty_bit = true;
	}

	void RayPass::node_update(uint32_t id) {
		_nodes[id] = RayPassNode::create(_scene->get_node(id), this);
		_node_dirty_bit = true;
	}

	void RayPass::node_remove(uint32_t id) {}

	util::Result<void, KError> RayPass::_create_descriptor_sets() {
		auto descriptor_templates = std::vector<DescriptorSetTemplate>();
		auto textures = used_textures();

		descriptor_templates.push_back(DescriptorSetTemplate::create_image_target(
					0, 
					VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					_result_image_view));

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					1,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_mapped_uniform));

		if (auto buffer = DescriptorSetTemplate::create_storage_buffer(
					2,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_vertex_buffer))
		{
			descriptor_templates.push_back(buffer.value());
		} else {
			LOG_ERROR << "Problem creating vertex buffer: " << buffer.error() << std::endl;
		}

		if (auto buffer = DescriptorSetTemplate::create_storage_buffer(
					3,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_bvnode_buffer))
		{
			descriptor_templates.push_back(buffer.value());
		} else {
			LOG_ERROR << "Problem creating bvnode buffer: " << buffer.error() << std::endl;
		}

		if (auto buffer = DescriptorSetTemplate::create_storage_buffer(
					4,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_node_buffer))
		{
			descriptor_templates.push_back(buffer.value());
		} else {
			LOG_ERROR << "Problem creating node buffer: " << buffer.error() << std::endl;
		}

		if (textures.size() > 0) {
			if (auto images = DescriptorSetTemplate::create_images(
						5, 
						VK_SHADER_STAGE_COMPUTE_BIT, 
						std::vector<VkImageView>(textures.begin(), textures.end())))
			{
				descriptor_templates.push_back(images.value());
			} else {
				LOG_ERROR << "Problem attaching images: " << images.error() << std::endl;
			}
		}

		if (auto buffer = DescriptorSetTemplate::create_storage_buffer(
					6,
					VK_SHADER_STAGE_COMPUTE_BIT,
					_material_buffer))
		{
			descriptor_templates.push_back(buffer.value());
		} else {
			LOG_ERROR << "Problem creating material buffer: " << buffer.error() << std::endl;
		}

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				1,
				_descriptor_pool);

		TRY(descriptor_sets);
		_descriptor_set = std::move(descriptor_sets.value());

		return {};
	}

	util::Result<void, KError> RayPass::_create_pipeline() {
		auto compute_shader = Shader::from_source_code(_codegen(used_textures().size()), Shader::Type::Compute);
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

	void RayPass::_update_buffers() {
		bool update = false;
		if (_vertex_dirty_bit) {
			_create_mesh_buffers();
			_vertex_dirty_bit = false;
			_node_dirty_bit = true;
			update = true;
		}

		if (_node_dirty_bit) {
			_create_node_buffers();
			_node_dirty_bit = false;
			_material_dirty_bit = true;
			update = true;
		}

		if (_material_dirty_bit) {
			_create_material_buffers();
			_material_dirty_bit = false;
			update = true;
		}

		if (update) {
			_create_descriptor_sets();
		}
	}

	void RayPass::_create_mesh_buffers() {
		/* setup buffer on cpu */
		auto vertices = std::vector<vulkan::Vertex>();
		auto bvnodes = std::vector<BVNode>();
		bvnodes.push_back(BVNode::create_empty());
		for (auto &mesh : _meshes) {
			mesh.build(bvnodes, vertices);
		}
		for (auto &bvnode : bvnodes) {
		}
		if (vertices.empty()) {
			//make sure buffer isn't empty because vulkan
			vertices.push_back(vulkan::Vertex());
		}
		for (auto &vertex : vertices) {
		}

		if (auto buffer = StaticBuffer::create(vertices)) {
			_vertex_buffer = std::move(buffer.value());
		} else {
			if (buffer.error().type() != KError::Type::EMPTY_BUFFER) {
				LOG_ERROR << buffer.error() << std::endl;
			}
		}

		if (auto buffer = StaticBuffer::create(bvnodes)) {
			_bvnode_buffer = std::move(buffer.value());
		} else {
			if (buffer.error().type() != KError::Type::EMPTY_BUFFER) {
				LOG_ERROR << buffer.error() << std::endl;
			}
		}
	}

	void RayPass::_create_node_buffers() {
		auto nodes = std::vector<RayPassNode::VImpl>();
		for (auto &node : _nodes) {
			nodes.push_back(node.vimpl());
		}

		if (nodes.empty()) {
			nodes.push_back(RayPassNode::VImpl::create_empty());
		}

		if (auto buffer = StaticBuffer::create(nodes)) {
			_node_buffer = std::move(buffer.value());
		} else {
			if (buffer.error().type() != KError::Type::EMPTY_BUFFER) {
				LOG_ERROR << buffer.error() << std::endl;
			}
		}
	}

	void RayPass::_create_material_buffers() {
		for (auto &m : _materials) {
			m.update(); //TODO: keep track of a dirty bit
		}

		auto range = max_material_range() * _nodes.size();
		if (range == 0) {
			range = 1;
		}
		auto buf = std::vector<char>(range);
		size_t i = 0;
		for (auto &node : _nodes) {
			auto &n = node.get();
			n.material().resources().update_prim_uniform(
					buf.data() + i * max_material_range(),
					n.resources().begin(),
					n.resources().end());
			i++;
		}
		if (auto buffer = StaticBuffer::create(buf.data(), range)) {
			_material_buffer = std::move(buffer.value());
		} else {
			LOG_ERROR << buffer.error() << std::endl;
		}
	}

	std::string RayPass::_codegen(uint32_t texture_count) {
		auto source = util::readEnvFile("assets/default_shader.comp");


		auto resource_decls = std::string();
		auto material_bufs = std::string();
		auto material_srcs = std::string();
		for (auto &material : _materials) {
			resource_decls += material.cg_struct_decl() + "\n";
			material_bufs += material.cg_buf_decl();
			material_srcs += material.cg_frag_def();
		}

		auto material_call = std::string();
		bool first_call = true;
		for (auto &material : _materials) {
			auto id = std::to_string(material.get()->id());
			if (first_call) {
				first_call = false;
			} else {
				material_call += " else ";
			}
			material_call += "if (nodes[node_id].material_id == " + id + ") {\n";
			material_call += "\t\t\t\t" + material.cg_frag_call() + ";\n";
			material_call += "\t\t\t}\n";
		}

		util::replace_substr(source, "/*VERTEX_DECL*/\n", Vertex::declaration());
		util::replace_substr(source, "/*BVNODE_DECL*/\n", BVNode::declaration());
		util::replace_substr(source, "/*NODE_DECL*/\n", RayPassNode::VImpl::declaration());
		util::replace_substr(source, "/*RESOURCE_DECL*/", resource_decls);
		util::replace_substr(source, "/*UNIFORM_DECL*/\n", ComputeUniformBuffer::declaration());
		util::replace_substr(source, "/*TEXTURE_COUNT*/", std::to_string(texture_count));
		util::replace_substr(source, "/*MATERIAL_BUFFERS*/\n", material_bufs);
		util::replace_substr(source, "/*MATERIAL_SRCS*/\n", material_srcs);
		util::replace_substr(source, "/*MATERIAL_CALLS*/\n", material_call);

		std::string source_log = source;
		util::add_strnum(source_log);
		LOG_DEBUG << "codegen: \n" << source_log << std::endl;

		return source;
	}

}
