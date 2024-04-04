#include <array>
#include <optional>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <glm/vector_relational.hpp>

#include "textureMaterial.hpp"
#include "defs.hpp"
#include "graphics.hpp"
#include "PreviewRenderPass.hpp"
#include "shader.hpp"
#include "uniformBufferObject.hpp"
#include "vertex.hpp"

namespace vulkan {
	util::Result<TextureMaterialPrevImpl, KError> TextureMaterialPrevImpl::create(
			PreviewRenderPass &render_pass,
			Texture *texture)
	{
		auto result = TextureMaterialPrevImpl(render_pass);
		result._texture = texture;

		/* Create Descriptor Set Layout */
		auto ubo_layout_binding = VkDescriptorSetLayoutBinding{};
		ubo_layout_binding.binding = 0;
		ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_layout_binding.descriptorCount = 1;
		ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		ubo_layout_binding.pImmutableSamplers = nullptr;

		auto sampler_layout_binding = VkDescriptorSetLayoutBinding{};
		sampler_layout_binding.binding = 1;
		sampler_layout_binding.descriptorCount = 1;
		sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_layout_binding.pImmutableSamplers = nullptr;
		sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		auto bindings = std::array<VkDescriptorSetLayoutBinding, 2>{
			ubo_layout_binding,
			sampler_layout_binding,
		};
		auto layout_info = VkDescriptorSetLayoutCreateInfo{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
		layout_info.pBindings = bindings.data();

		auto res = vkCreateDescriptorSetLayout(
				Graphics::DEFAULT->device(), 
				&layout_info, nullptr, 
				&result._descriptor_set_layout);
		if (res != VK_SUCCESS) {
			return {res};
		}

		/* Set up descriptor sets */
		auto layouts = std::vector<VkDescriptorSetLayout>(
				FRAMES_IN_FLIGHT,
				result._descriptor_set_layout);
		auto alloc_info = VkDescriptorSetAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		//TODO
		alloc_info.descriptorPool = render_pass.descriptor_pool().descriptorPool();
		alloc_info.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
		alloc_info.pSetLayouts = layouts.data();

		result._descriptor_sets.resize(FRAMES_IN_FLIGHT);
		res = vkAllocateDescriptorSets(
				Graphics::DEFAULT->device(), 
				&alloc_info, 
				result._descriptor_sets.data());
		if (res != VK_SUCCESS) {
			return {res};
		}

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto buffer_info = VkDescriptorBufferInfo{};
			buffer_info.buffer = render_pass.uniform_buffers()[i].buffer();
			buffer_info.offset = 0;
			buffer_info.range = sizeof(UniformBufferObject);

			auto image_info = VkDescriptorImageInfo{};
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.imageView = texture->image_view().value();
			image_info.sampler = Graphics::DEFAULT->mainTextureSampler();

			auto descriptor_writes = std::array<VkWriteDescriptorSet, 2>();

			descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[0].dstSet = result._descriptor_sets[i];
			descriptor_writes[0].dstBinding = 0;
			descriptor_writes[0].dstArrayElement = 0;
			descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor_writes[0].descriptorCount = 1;
			descriptor_writes[0].pBufferInfo = &buffer_info;

			descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptor_writes[1].dstSet = result._descriptor_sets[i];
			descriptor_writes[1].dstBinding = 1;
			descriptor_writes[1].dstArrayElement = 0;
			descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptor_writes[1].descriptorCount = 1;
			descriptor_writes[1].pImageInfo = &image_info;

			vkUpdateDescriptorSets(
					Graphics::DEFAULT->device(), 
					static_cast<uint32_t>(descriptor_writes.size()), 
					descriptor_writes.data(), 
					0,
					nullptr);
		}

		/* Create pipeline */
		auto vert_shader = vulkan::Shader::fromEnvFile(
				"src/shaders/default_shader.vert.spv");
		auto frag_shader = vulkan::Shader::fromEnvFile(
				"src/shaders/default_shader.frag.spv");

		auto shader_stages = std::array<VkPipelineShaderStageCreateInfo, 2>();

		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = vert_shader.shaderModule();
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = frag_shader.shaderModule();
		shader_stages[1].pName = "main";

		auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{};

		auto binding_description = Vertex::getBindingDescription();
		auto attribute_descriptions = Vertex::getAttributeDescriptions();

		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(
				attribute_descriptions.size());
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

		auto color_blend_attachment = VkPipelineColorBlendAttachmentState{};
		color_blend_attachment.colorWriteMask = 
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		auto color_blending = VkPipelineColorBlendStateCreateInfo{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;
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

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &result._descriptor_set_layout;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		res = vkCreatePipelineLayout(
				Graphics::DEFAULT->device(),
				&pipeline_layout_info,
				nullptr,
				&result._pipeline_layout);
		if (res != VK_SUCCESS) {
			return {res};
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
		pipeline_info.layout = result._pipeline_layout;
		pipeline_info.renderPass = render_pass.render_pass();
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		res = vkCreateGraphicsPipelines(
				Graphics::DEFAULT->device(), 
				VK_NULL_HANDLE,
				1, 
				&pipeline_info,
				nullptr, 
				&result._pipeline);

		if (res != VK_SUCCESS) {
			return {res};
		}

		return std::move(result);
	}

	TextureMaterialPrevImpl::TextureMaterialPrevImpl(TextureMaterialPrevImpl &&other):
		vulkan::TextureMaterialPrevImpl(other._render_pass)
	{
		_texture = other._texture;

		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_set_layout = other._descriptor_set_layout;
		other._descriptor_set_layout = nullptr;
	}

	TextureMaterialPrevImpl& TextureMaterialPrevImpl::operator=(TextureMaterialPrevImpl&& other) {
			_texture = other._texture;

		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_set_layout = other._descriptor_set_layout;
		other._descriptor_set_layout = nullptr;
	
		return *this;
	}

	TextureMaterialPrevImpl::~TextureMaterialPrevImpl() {
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
		if (_descriptor_set_layout) {
			vkDestroyDescriptorSetLayout(
					Graphics::DEFAULT->device(),
					_descriptor_set_layout,
					nullptr);
			_descriptor_set_layout = nullptr;
		}
		
		if (_descriptor_sets.size() > 0) {
			vkFreeDescriptorSets(
					Graphics::DEFAULT->device(),
					_render_pass.descriptor_pool().descriptorPool(),
					_descriptor_sets.size(),
					_descriptor_sets.data());
			_descriptor_sets.clear();
		}
	}

	VkPipelineLayout TextureMaterialPrevImpl::pipeline_layout() {
		return _pipeline_layout;
	}

	VkPipeline TextureMaterialPrevImpl::pipeline() {
		return _pipeline;
	}

	VkDescriptorSet TextureMaterialPrevImpl::get_descriptor_set(uint32_t frame_index) {
		return _descriptor_sets[frame_index];
	}

	TextureMaterialPrevImpl::TextureMaterialPrevImpl(PreviewRenderPass &render_pass):
		_texture(nullptr),
		_pipeline_layout(nullptr),
		_pipeline(nullptr),
		_descriptor_set_layout(nullptr),
		_render_pass(render_pass)
	{}

	TextureMaterial::TextureMaterial(Texture* texture):
	_texture(texture)
	{}

	util::Result<void, KError> TextureMaterial::add_preview(
			PreviewRenderPass &preview_render_pass)
	{
		auto res = TextureMaterialPrevImpl::create(preview_render_pass, _texture);
		if (res) {
			_preview_impl = std::move(res.value());
			return {};
		} else {
			return res.error();
		}
	}

	MaterialPreviewImpl *TextureMaterial::preview_impl() {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}

	MaterialPreviewImpl const *TextureMaterial::preview_impl() const {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}
}
