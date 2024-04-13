#include <array>

#include <vulkan/vulkan_core.h>

#include "ColorMaterial.hpp"
#include "defs.hpp"
#include "shader.hpp"
#include "vertex.hpp"

namespace vulkan {
	util::Result<ColorMaterialPrevImpl, KError> ColorMaterialPrevImpl::create(
			PreviewRenderPass &render_pass,
			glm::vec3 color)
	{
		auto result = ColorMaterialPrevImpl(render_pass.descriptor_pool());
		result._color = color;

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto buffer_res = MappedUniformObject::create();
			TRY(buffer_res);
			result._mapped_uniforms.push_back(std::move(buffer_res.value()));
		}
		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto color_buf_res = ColorUniformObject::create();
			TRY(color_buf_res);
			result._color_uniforms.push_back(std::move(color_buf_res.value()));
		}

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					0,
					VK_SHADER_STAGE_VERTEX_BIT,
					result._mapped_uniforms));
		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(
					1, VK_SHADER_STAGE_FRAGMENT_BIT,
					result._color_uniforms));

		auto descriptor_sets = DescriptorSets::create(
				descriptor_templates,
				FRAMES_IN_FLIGHT,
				render_pass.descriptor_pool());
		TRY(descriptor_sets);
		result._descriptor_sets = std::move(descriptor_sets.value());

		/* Create pipeline */
		auto vert_shader = vulkan::Shader::fromEnvFile(
				"src/shaders/default_shader.vert.spv");
		auto frag_shader = vulkan::Shader::fromEnvFile(
				"src/shaders/color_shader.frag.spv");

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
		pipeline_layout_info.pSetLayouts = result._descriptor_sets.layout_ptr();
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		auto res = vkCreatePipelineLayout(
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

	ColorMaterialPrevImpl::ColorMaterialPrevImpl(ColorMaterialPrevImpl &&other):
		ColorMaterialPrevImpl(other._descriptor_sets.descriptor_pool())
	{
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_mapped_uniforms = std::move(other._mapped_uniforms);

		_color_uniforms = std::move(other._color_uniforms);

		_color = other._color;
	}

	ColorMaterialPrevImpl& ColorMaterialPrevImpl::operator=(ColorMaterialPrevImpl&& other)
	{
		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		_descriptor_sets = std::move(other._descriptor_sets);

		_mapped_uniforms = std::move(other._mapped_uniforms);

		_color_uniforms = std::move(other._color_uniforms);

		_color = other._color;

		return *this;
	}

	ColorMaterialPrevImpl::~ColorMaterialPrevImpl() {
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
	}

	VkPipelineLayout ColorMaterialPrevImpl::pipeline_layout() {
		return _pipeline_layout;
	}

	VkPipeline ColorMaterialPrevImpl::pipeline() {
		return _pipeline;
	}

	VkDescriptorSet ColorMaterialPrevImpl::get_descriptor_set(uint32_t frame_index) {
		return _descriptor_sets.descriptor_set(frame_index);
	}

	void ColorMaterialPrevImpl::update_uniform(
			uint32_t frame_index,
			glm::vec3 position,
			glm::vec2 viewport_size)
	{
		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

		auto ubo = UniformBufferObject{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.model = glm::translate(ubo.model, position);
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), viewport_size.x / (float) viewport_size.y, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		_mapped_uniforms[frame_index].set_value(ubo);

		auto color = ColorBufferObject{_color};
		color.color.y += 0.2 + sin(time * 10) * 0.2;
		_color_uniforms[frame_index].set_value(color);
	}


	ColorMaterialPrevImpl::ColorMaterialPrevImpl(DescriptorPool &descriptor_pool):
		_descriptor_sets(descriptor_pool)
	{ }

	ColorMaterial::ColorMaterial(glm::vec3 color):
		_color(color)
	{ }

	util::Result<void, KError> ColorMaterial::add_preview(
			PreviewRenderPass &preview_render_pass)
	{
		auto res = ColorMaterialPrevImpl::create(preview_render_pass, _color);
		if (res) {
			_preview_impl = std::move(res.value());
			return {};
		} else {
			return res.error();
		}
	}

	ColorMaterialPrevImpl *ColorMaterial::preview_impl() {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}

	ColorMaterialPrevImpl const *ColorMaterial::preview_impl() const {
		if (_preview_impl) {
			return &_preview_impl.value();
		} else {
			return nullptr;
		}
	}

}
