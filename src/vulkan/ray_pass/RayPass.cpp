#include <memory>
#include <vulkan/vulkan_core.h>
#include <imgui_impl_vulkan.h>
#include <random>

#include "RayPass.hpp"
#include "RayPassMaterial.hpp"
#include "RayPassMesh.hpp"
#include "RayPassNode.hpp"
#include "../PassUtil.hpp"
#include "codegen/TemplGen.hpp"
#include "util/log.hpp"
#include "util/result.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/graphics.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/Vertex.hpp"
#include "types/Node.hpp"
#include "util/file.hpp"
#include "util/Util.hpp"

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

	util::Result<RayPass::Ptr, RayPass::Error> RayPass::create(
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

		if (auto err = Fence::create().move_or(result->_pass_fence)) {
			return Error(ErrorType::VULKAN, "Could not create ray pass fence", {err.value()});
		}

		if (auto err = Semaphore::create().move_or(result->_semaphore)) {
			return Error(ErrorType::VULKAN, "Could not create ray pass semaphore", {err.value()});
		}

		TRY(result->_create_images());

		if (auto err = MappedComputeUniform::create().move_or(result->_mapped_uniform)) {
			return Error(ErrorType::VULKAN, "Could not create mapped uniform", {err.value()});
		}

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
			return Error(ErrorType::VULKAN, "Could not allocate command buffers", {res});
		}

		result->_vertex_dirty_bit = true;
		result->_node_dirty_bit = true;
		result->_material_dirty_bit = true;
		result->_clear_accumulator = true;

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
		_accumulator_image = std::move(other._accumulator_image);
		_pass_fence = std::move(other._pass_fence);
		_semaphore = std::move(other._semaphore);
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

		_mesh_observer = std::move(other._mesh_observer);

		_material_observer = std::move(other._material_observer);

		_node_observer = std::move(other._node_observer);

		_vertex_dirty_bit = other._vertex_dirty_bit;
		_node_dirty_bit = other._node_dirty_bit;
		_material_dirty_bit = other._material_dirty_bit;

		_vertex_buffer = std::move(other._vertex_buffer);

		_bvnode_buffer = std::move(other._bvnode_buffer);

		_node_buffer = std::move(other._node_buffer);

		_material_buffer = std::move(other._material_buffer);

		_meshes = std::move(other._meshes);

		_nodes = std::move(other._nodes);

		_materials = std::move(other._materials);

		_scene = other._scene;

		_compute_index = other._compute_index;
		_ray_count = other._ray_count;
		_clear_accumulator = other._clear_accumulator;
	}

	RayPass& RayPass::operator=(RayPass&& other) {
		this->~RayPass();

		_size = other._size;
		_result_image = std::move(other._result_image);
		_accumulator_image = std::move(other._accumulator_image);
		_pass_fence = std::move(other._pass_fence);
		_semaphore = std::move(other._semaphore);
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
		_mesh_observer = std::move(other._mesh_observer);
		_material_observer = std::move(other._material_observer);
		_node_observer = std::move(other._node_observer);

		_vertex_dirty_bit = other._vertex_dirty_bit;
		_node_dirty_bit = other._node_dirty_bit;
		_material_dirty_bit = other._material_dirty_bit;

		_vertex_buffer = std::move(other._vertex_buffer);
		_bvnode_buffer = std::move(other._bvnode_buffer);
		_node_buffer = std::move(other._node_buffer);
		_material_buffer = std::move(other._material_buffer);

		_meshes = std::move(other._meshes);
		_nodes = std::move(other._nodes);
		_materials = std::move(other._materials);

		_scene = other._scene;
		_compute_index = other._compute_index;
		_ray_count = other._ray_count;
		_clear_accumulator = other._clear_accumulator;
		
		return *this;
	}

	RayPass::RayPass():
	_pipeline_layout(nullptr),
	_pipeline(nullptr)
	{ }

	VkDescriptorSet RayPass::imgui_descriptor_set() {
		return _imgui_descriptor_set;
	}
	VkImageView RayPass::image_view() {
		return _result_image.image_view();
	}

	VkSemaphore RayPass::submit(
			Node &node,
			uint32_t count,
			ComputeUniform uniform,
			VkSemaphore semaphore)
	{
		auto rand = std::random_device();
		auto dist = std::uniform_int_distribution<uint32_t>();
		static glm::u32vec4 seed = {1919835750, 2912171293, 1124614627, 4259748986};

		_update_buffers();
		if (_descriptor_set.has_value()) {
			if (auto err = _create_descriptor_sets().move_or()) {
				log_error() << err.value() << std::endl;
			}
		}
		if (!_pipeline) {
			auto res = _create_pipeline();
			if (!res) {
				log_error() << "problem creating pipeline: " << res.error() << std::endl;
				return nullptr;
			}
		}

		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		vkWaitForFences(Graphics::DEFAULT->device(), 1, &_pass_fence.get(), VK_TRUE, UINT64_MAX);
		vkResetFences(Graphics::DEFAULT->device(), 1, &_pass_fence.get());

		vkResetCommandBuffer(_command_buffer, 0);

		uniform.seed = seed;
		uniform.ray_count = _ray_count;
		uniform.compute_index = _compute_index;

		_compute_index += count;
		uint32_t pixel_count = _result_image.width() * _result_image.height();
		if (_compute_index > pixel_count) {
			seed.x = dist(rand);
			seed.y = dist(rand);
			seed.z = dist(rand);
			seed.w = dist(rand);
			count = count - (_compute_index - pixel_count);
			_compute_index = 0;
			_ray_count++;
		}
		
		_mapped_uniform.set_value(uniform);

		auto begin_info = VkCommandBufferBeginInfo{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(_command_buffer, &begin_info) != VK_SUCCESS) {
			log_error() << "Couldn't begin command buffer" << std::endl;
		}
		VkClearColorValue clear_color = {0.0, 0.0, 0.0, 0.0};
		VkImageSubresourceRange range;

		if (_clear_accumulator) {
				_clear_accumulator = false;
			//TODO: this information is copied from the image view constructor
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = 1;
			range.baseArrayLayer = 0;
			range.layerCount = 1;
			range.levelCount = 1;
			vkCmdClearColorImage(_command_buffer, _accumulator_image.image(), VK_IMAGE_LAYOUT_GENERAL, &clear_color, 1, &range);
			log_debug() << "Cleared accumulator" << std::endl;
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
		vkCmdDispatch(_command_buffer, count, 1, 1);
		if (vkEndCommandBuffer(_command_buffer) != VK_SUCCESS) {
			log_error() << "Couldn't end command buffer" << std::endl;
		}

		auto ray_semaphore = _semaphore.get();

		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &_command_buffer;
		if (semaphore) {
			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT; 
			submit_info.pWaitSemaphores = &semaphore;
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitDstStageMask = &wait_stage;
		}
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &ray_semaphore;

		auto res = vkQueueSubmit(Graphics::DEFAULT->compute_queue(), 1, &submit_info, *_pass_fence);
		if (res != VK_SUCCESS) {
			log_error() << "Problem submitting queue" << std::endl;
		}
		return ray_semaphore;
	}

	MappedComputeUniform &RayPass::current_uniform_buffer() {
		return _mapped_uniform;
	}

	void RayPass::resize(VkExtent2D size) {
		if (size.width == _size.width && size.height == _size.height) return;
		_size = size;
		_size_dirty_bit = true;
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

		for (auto &t : _scene->resource_manager().textures().raw()) {
			if (t) {
				result.push_back(t->image_view());
			} else {
				result.push_back(nullptr);
			}
		}

		return result;
	}

	void RayPass::reset_counters() {
		_compute_index = 0;
		_ray_count = 1;
		_clear_accumulator = true;
	}
	
	void RayPass::mesh_create(uint32_t id) {
		_meshes.insert(RayPassMesh(_scene->resource_manager().get_mesh(id), this));
		_vertex_dirty_bit = true;
	}

	void RayPass::mesh_update(uint32_t id) {
		_vertex_dirty_bit = true;
	}

	void RayPass::mesh_remove(uint32_t id) {}

	void RayPass::material_create(uint32_t id) {
		if (auto material = RayPassMaterial::create(_scene->resource_manager().get_material(id), this)) {
			log_assert(_materials.insert(std::move(material.value())), "Duplicate material in RayPass");
		} else {
			log_error() << "Could not create ray pass material: " << std::endl
			<< material.error();
		}
		_material_dirty_bit = true;
	}

	void RayPass::material_update(uint32_t id) {
		if (auto material = RayPassMaterial::create(_scene->resource_manager().get_material(id), this)) {
			_materials[id] = std::move(material.value());
		} else {
			log_error() << "Could not update ray pass material: " << std::endl
			<< material.error();
		}
		_material_dirty_bit = true;
	}

	void RayPass::material_remove(uint32_t id) {
	}

	void RayPass::node_create(uint32_t id) {
		auto node = _scene->get_node(id);
		if (node->type() != vulkan::Node::Type::Object) return;

		if (auto rt_node = RayPassNode::create(node, this)) {
			log_assert(_nodes.insert(std::move(rt_node.value())), "Duplicate node in RayPass");
		} else {
			TRY_LOG(rt_node);
		}
		_node_dirty_bit = true;
	}

	void RayPass::node_update(uint32_t id) {
		auto node = _scene->get_node(id);
		if (node->type() != vulkan::Node::Type::Object) return;

		if (auto rt_node = RayPassNode::create(node, this)) {
			_nodes[id] = std::move(rt_node.value());
		} else {
			TRY_LOG(rt_node);
		}
		_node_dirty_bit = true;
	}

	void RayPass::node_remove(uint32_t id) {
		_nodes[id].destroy();
		_node_dirty_bit = true;
	}

	util::Result<void, RayPass::Error> RayPass::_create_descriptor_sets() {
		auto textures = used_textures();
		auto bindings = std::vector<VkDescriptorSetLayoutBinding>();

		bindings.push_back(descriptor_layout_image_target(VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1));
		bindings.push_back(descriptor_layout_image_target(VK_SHADER_STAGE_COMPUTE_BIT, 1));
		bindings.push_back(descriptor_layout_uniform(VK_SHADER_STAGE_COMPUTE_BIT));
		bindings.push_back(descriptor_layout_storage_buffer(VK_SHADER_STAGE_COMPUTE_BIT));
		bindings.push_back(descriptor_layout_storage_buffer(VK_SHADER_STAGE_COMPUTE_BIT));
		bindings.push_back(descriptor_layout_storage_buffer(VK_SHADER_STAGE_COMPUTE_BIT));
		bindings.push_back(descriptor_layout_storage_buffer(VK_SHADER_STAGE_COMPUTE_BIT));
		if (textures.size() > 0) {
			bindings.push_back(descriptor_layout_images(VK_SHADER_STAGE_COMPUTE_BIT, textures.size()));
		}

		_descriptor_set_layout = DescriptorSetLayout::create(bindings).move_value();

		auto builder = _descriptor_set_layout.builder();

		if (auto err = builder.add_image_target(_result_image.image_view()).move_or()) {
			return Error(ErrorType::SHADER_RESOURCES, "Could not add result image", err.value());
		}
		if (auto err = builder.add_image_target(_accumulator_image.image_view()).move_or()) {
			return Error(ErrorType::SHADER_RESOURCES, "Could not add accumulator image", err.value());
		}
		if (auto err = builder.add_uniform(_mapped_uniform).move_or()) {
			return Error(ErrorType::SHADER_RESOURCES, "Could not add mapped uniform", err.value());
		}
		if (auto err = builder.add_storage_buffer(_vertex_buffer).move_or()) {
			return Error(ErrorType::SHADER_RESOURCES, "Could not add vertex buffer", err.value());
		}
		if (auto err = builder.add_storage_buffer(_bvnode_buffer).move_or()) {
			return Error(ErrorType::SHADER_RESOURCES, "Could not add bvnode buffer", err.value());
		}
		if (auto err = builder.add_storage_buffer(_node_buffer).move_or()) {
			return Error(ErrorType::SHADER_RESOURCES, "Could not add node buffer", err.value());
		}
		if (auto err = builder.add_storage_buffer(_material_buffer).move_or()) {
			return Error(ErrorType::SHADER_RESOURCES, "Could not add material buffer", err.value());
		}
		if (textures.size() > 0) {
			if (auto err = builder.add_image(textures).move_or()) {
				return Error(ErrorType::SHADER_RESOURCES, "Could not add ray pass textures", err.value());
			}
		}

		_descriptor_set = DescriptorSets::create(builder, _descriptor_pool).move_value();

		return {};
	}

	util::Result<void, RayPass::Error> RayPass::_create_pipeline() {
		auto compute_shader = Shader::from_source_code(_codegen(used_textures().size()), Shader::Type::Compute);
		if (!compute_shader) {
			return Error(ErrorType::VULKAN, "Could not compile compute shader code", compute_shader.error());
		}

		auto compute_shader_stage_info = VkPipelineShaderStageCreateInfo{};
		compute_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		compute_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		compute_shader_stage_info.module = compute_shader.value().shader_module();
		compute_shader_stage_info.pName = "main";

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &_descriptor_set_layout.layout();

		auto res = vkCreatePipelineLayout(Graphics::DEFAULT->device(), &pipeline_layout_info, nullptr, &_pipeline_layout);
		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create pipeline layout for ray pass", {res});
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
			&_pipeline
		);
		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create compute pipline for ray pass", {res});
		}

		log_debug() << "just created raypass pipeline " << _pipeline << std::endl;

		return {};
	}

	void RayPass::_cleanup_images() {
		_result_image.destroy();
		_result_image.destroy();
		_accumulator_image.destroy();
		_accumulator_image.destroy();
	}

	util::Result<void, RayPass::Error> RayPass::_create_images() {
		// result image
		if (auto err = Image::create(
			_size,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
				| VK_IMAGE_USAGE_STORAGE_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT
			).move_or(_result_image)
		) {
			return Error(ErrorType::VULKAN, "Could not create result image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_result_image.image(),
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			1
		);

		_imgui_descriptor_set = ImGui_ImplVulkan_AddTexture(
			*Graphics::DEFAULT->main_texture_sampler(), 
			_result_image.image_view(), 
			VK_IMAGE_LAYOUT_GENERAL
		);
		log_assert(_imgui_descriptor_set, "imgui descriptor set must be initialized");

		//Setup accumulator
		if (auto err = Image::create(
			_size,
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_USAGE_STORAGE_BIT
				| VK_IMAGE_USAGE_SAMPLED_BIT
				| VK_IMAGE_USAGE_TRANSFER_DST_BIT) .move_or(_accumulator_image)
		) {
			return Error(ErrorType::VULKAN, "Could not create accumulator image", err.value());
		}

		Graphics::DEFAULT->transition_image_layout(
			_accumulator_image.image(), 
			VK_FORMAT_R16G16B16A16_SFLOAT, 
			VK_IMAGE_LAYOUT_UNDEFINED, 
			VK_IMAGE_LAYOUT_GENERAL, 
			1
		);

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
			log_debug() << "update node buffers " << std::endl;
			_create_node_buffers();
			_node_dirty_bit = false;
			_material_dirty_bit = true;
			update = true;
		}

		if (_material_dirty_bit) {
			//_create_material_buffers();
			_material_buffer = std::move(create_material_buffer(*_scene).value());
			_material_dirty_bit = false;
			update = true;
		}

		if (_size_dirty_bit) {
			_cleanup_images();
			if (auto err = _create_images().move_or()) {
				log_error() << "Could not create images: " << err.value() << std::endl;
			}
			_size_dirty_bit = false;
			update = true;
		}

		if (update) {
			if (auto err = _create_descriptor_sets().move_or()) {
				log_error() << "Could not create descriptor set: " << err.value() << std::endl;
			}
			//TODO: Slow as it doesn't need to be done every time
			if (auto err = _create_pipeline().move_or()) {
				log_error() << "Could not create pipeline: " << err.value() << std::endl;
			}
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
			if (buffer.error().type() != vulkan::ErrorType::EMPTY_BUFFER) {
				log_error() << buffer.error() << std::endl;
			}
		}

		if (auto buffer = StaticBuffer::create(bvnodes)) {
			_bvnode_buffer = std::move(buffer.value());
		} else {
			if (buffer.error().type() != vulkan::ErrorType::EMPTY_BUFFER) {
				log_error() << buffer.error() << std::endl;
			}
		}
	}

	void RayPass::_create_node_buffers() {
		auto nodes = std::vector<RayPassNode::VImpl>();
		for (auto &node : _nodes.raw()) {
			nodes.push_back(node.vimpl());
		}

		if (nodes.empty()) {
			nodes.push_back(RayPassNode::VImpl::create_empty());
		}

		if (auto buffer = StaticBuffer::create(nodes)) {
			_node_buffer = std::move(buffer.value());
		} else {
			if (buffer.error().type() != vulkan::ErrorType::EMPTY_BUFFER) {
				log_error() << buffer.error() << std::endl;
			}
		}
	}

	std::string RayPass::_codegen(uint32_t texture_count) {
		auto source = util::readEnvFile("assets/shaders/raytrace.comp.cg");

		auto materials = cg::TemplList();
		for (auto &material : _materials) {
			materials.push_back(material_templobj(
				material.id(),
				_scene->resource_manager().materials()
			));
		}
		auto meshes = cg::TemplList();
		for (auto &mesh : _meshes) {
			meshes.push_back(mesh.base_mesh()->cg_templobj());
		}

		auto args = cg::TemplObj{
			{"vertex_declarations", Vertex::declaration},
			{"bvnode_declarations", BVNode::declaration},
			{"bvnode_defines", BVNode::defines},
			{"node_declarations", RayPassNode::VImpl::declaration},
			{"materials", materials},
			{"meshes", meshes},
			{"global_declarations", ComputeUniform::declarations},
			{"texture_count", texture_count}
		};

		auto start = log_start_timer();
		source = cg::TemplGen::codegen(source, args, "raytrace.comp.cg").value();
		log_debug() << "raypass codegen took " << start << std::endl;

		log_debug() << "raytrace codegen: \n" << util::add_strnum(source) << std::endl;

		return source;
	}
}

template<>
const char *vulkan::RayPass::Error::type_str(vulkan::RayPass::ErrorType t) {
	switch (t) {
		case vulkan::RayPass::ErrorType::VULKAN:
			return "RayPass.VULKAN";
		case vulkan::RayPass::ErrorType::SHADER_RESOURCES:
			return "RayPass.SHADER_RESOURCES";
		case vulkan::RayPass::ErrorType::MISC:
			return "RayPass.MISC";
	}
}
