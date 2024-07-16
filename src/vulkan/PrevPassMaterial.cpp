#include <array>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "PrevPass.hpp"
#include "PrevPassMaterial.hpp"
#include "../vulkan/Vertex.hpp"
#include "../util/file.hpp"
#include "../util/Util.hpp"
#include "../types/Material.hpp"

namespace vulkan {
	util::Result<PrevPassMaterial, KError> PrevPassMaterial::create(
			Scene &scene,
			PrevPass &preview_pass,
			const types::Material *material)
	{
		auto result = PrevPassMaterial();
		result._material = material;

		result._render_pass = &preview_pass;
		
		/* code gen vertex code */
		auto vert_source = std::string();
		auto frag_source = std::string();
		_codegen(frag_source, vert_source, material);

		auto vert_shader = vulkan::Shader::from_source_code(vert_source, Shader::Type::Vertex);
		TRY(vert_shader);
		auto frag_shader = vulkan::Shader::from_source_code(frag_source, Shader::Type::Fragment);
		TRY(frag_shader);

		auto layout_bindings = std::vector<VkDescriptorSetLayoutBinding>(
				1,
				VkDescriptorSetLayoutBinding{});
		layout_bindings[0].binding = 0;
		layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layout_bindings[0].descriptorCount = 1;
		layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		layout_bindings[0].pImmutableSamplers = nullptr;

		bool has_image = false;
		for (auto &resource : material->resources()) {
			if (resource.type() == types::ShaderResource::Type::Image) {
				has_image = true;
			}
		}
		if (has_image) {
			auto binding = VkDescriptorSetLayoutBinding{};
			binding.binding = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			binding.pImmutableSamplers = nullptr;
			layout_bindings.push_back(binding);
		}

		auto layout_info = VkDescriptorSetLayoutCreateInfo{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
		layout_info.pBindings = layout_bindings.data();

		auto descriptor_layout = DescriptorSetLayout::create(layout_info);
		TRY(descriptor_layout);

		//Nodes need to be created first.
		auto descriptor_layouts = std::vector<VkDescriptorSetLayout>{
			result._render_pass->global_descriptor_set_layout(),
			descriptor_layout.value().layout(),
		};

		auto res = _create_pipeline(
				vert_shader.value(), 
				frag_shader.value(), 
				*result._render_pass,
				descriptor_layouts,
				&result._pipeline,
				&result._pipeline_layout);
		LOG_MEMORY << "Just created a pipeline " << result._pipeline << std::endl;
		TRY(res);
		return result;
	}

	PrevPassMaterial::PrevPassMaterial(PrevPassMaterial &&other) {
		_material = other._material;
		other._material = nullptr;

		_render_pass = other._render_pass;
		other._render_pass = nullptr;

		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;
	}

	PrevPassMaterial& PrevPassMaterial::operator=(PrevPassMaterial &&other) {
		destroy();

		_render_pass = other._render_pass;
		other._render_pass = nullptr;

		_pipeline_layout = other._pipeline_layout;
		other._pipeline_layout = nullptr;

		_pipeline = other._pipeline;
		other._pipeline = nullptr;

		return *this;
	}

	void PrevPassMaterial::destroy() {
		_material = nullptr;
		_render_pass = nullptr;
		if (_pipeline_layout) {
			vkDestroyPipelineLayout(
					Graphics::DEFAULT->device(),
					_pipeline_layout,
					nullptr);
			_pipeline_layout = nullptr;
		}

		if (_pipeline) {
			vkDestroyPipeline(Graphics::DEFAULT->device(), _pipeline, nullptr);
			_pipeline = nullptr;
		}
	}

	PrevPassMaterial::~PrevPassMaterial() {
		destroy();
	}

	bool PrevPassMaterial::exists() const {
		return _pipeline != nullptr;
	}

	uint32_t PrevPassMaterial::id() const {
		return _material->id();
	}

	util::Result<void, KError> PrevPassMaterial::_create_pipeline(
			Shader &vertex_shader,
			Shader &fragment_shader,
			PrevPass &render_pass,
			std::vector<VkDescriptorSetLayout> &descriptor_set_layouts,
			VkPipeline *pipeline,
			VkPipelineLayout *pipeline_layout)
	{
		auto shader_stages = std::array<VkPipelineShaderStageCreateInfo, 2>();

		shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stages[0].module = vertex_shader.shader_module();
		shader_stages[0].pName = "main";

		shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stages[1].module = fragment_shader.shader_module();
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
		pipeline_layout_info.setLayoutCount = descriptor_set_layouts.size();
		pipeline_layout_info.pSetLayouts = descriptor_set_layouts.data();
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		auto res = vkCreatePipelineLayout(
				Graphics::DEFAULT->device(),
				&pipeline_layout_info,
				nullptr,
				pipeline_layout);
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
		pipeline_info.layout = *pipeline_layout;
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
				pipeline);

		if (res == VK_SUCCESS) {
			return {};
		} else {
			return {res};
		}
	}

	void PrevPassMaterial::_codegen(
			std::string &frag_source,
			std::string &vert_source,
			const types::Material *material)
	{
		frag_source = util::readEnvFile("assets/default_shader.frag");
		vert_source = util::readEnvFile("assets/default_shader.vert");
		auto uniform_source = std::string();
		for (auto &resource : material->resources()) {
			uniform_source += resource.declaration();
		}

		util::replace_substr(vert_source, "/*INSERT_MATERIAL_UNIFORM*/\n", uniform_source);
		util::replace_substr(frag_source, "/*INSERT_MATERIAL_UNIFORM*/\n", uniform_source);
		util::replace_substr(frag_source, "/*INSERT_FRAG_SRC*/\n", material->frag_shader_src());
	}
}
