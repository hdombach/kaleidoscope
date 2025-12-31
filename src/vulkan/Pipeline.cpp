#include "Pipeline.hpp"

#include <vulkan/vulkan_core.h>

#include "FrameAttachment.hpp"
#include "util/Util.hpp"

namespace vulkan {
	util::Result<Pipeline, Error> Pipeline::create_graphics(
		const Shader &vertex_shader,
		const Shader &fragment_shader,
		const RenderPass &render_pass,
		const Attachments &attachments
	) {
		auto pipeline = Pipeline();

		pipeline._type = Type::GRAPHICS;
		pipeline._render_pass = &render_pass;
		pipeline._attachments = attachments;

		auto shader_stage_infos = std::array<VkPipelineShaderStageCreateInfo, 2>();

		shader_stage_infos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_infos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shader_stage_infos[0].module = vertex_shader.shader_module();
		shader_stage_infos[0].pName = "main";

		shader_stage_infos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stage_infos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shader_stage_infos[1].module = fragment_shader.shader_module();
		shader_stage_infos[1].pName = "main";

		auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{};

		auto vertex_binding_description = VkVertexInputBindingDescription{};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(glm::vec4);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		auto vertex_attribute_descriptions = std::array<VkVertexInputAttributeDescription, 1>();
		vertex_attribute_descriptions[0].binding = 0;
		vertex_attribute_descriptions[0].location = 0;
		vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_attribute_descriptions[0].offset = 0;

		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &vertex_binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(
				vertex_attribute_descriptions.size());
		vertex_input_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

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

		auto blend_attachments = std::vector<VkPipelineColorBlendAttachmentState>();
		for (auto &attachemnt : render_pass.frame_attachments()) {
			blend_attachments.push_back({});
			if (auto err = attachemnt.blend_attachment_state().move_or(blend_attachments.back())) {
				return Error(
					ErrorType::MISC,
					"Could not resolve blend attachment",
					err.value()
				);
			}
		}

		auto color_blending = VkPipelineColorBlendStateCreateInfo{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = blend_attachments.size();
		color_blending.pAttachments = blend_attachments.data();
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

		for (auto &attachment_row : attachments) {
			pipeline._descriptor_set_layouts.push_back({});
			if (
				auto err = DescriptorSetLayout::create(attachment_row)
					.move_or(pipeline._descriptor_set_layouts.back())
			) {
				return Error(
					ErrorType::MISC,
					"Could not create descriptor set layout",
					err.value()
				);
			}
		}

		auto layouts = pipeline.vk_layouts();

		auto pipeline_layout_info = VkPipelineLayoutCreateInfo{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = layouts.size();
		pipeline_layout_info.pSetLayouts = layouts.data();
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		auto res = vkCreatePipelineLayout(
			Graphics::DEFAULT->device(),
			&pipeline_layout_info,
			nullptr,
			&pipeline._pipeline_layout
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
		pipeline_info.stageCount = shader_stage_infos.size();
		pipeline_info.pStages = shader_stage_infos.data();
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		//pipeline_info.pDepthStencilState = &depth_stencil;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_state;
		pipeline_info.layout = pipeline._pipeline_layout;
		pipeline_info.renderPass = pipeline._render_pass->render_pass();
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		res = vkCreateGraphicsPipelines(
			Graphics::DEFAULT->device(),
			VK_NULL_HANDLE,
			1,
			&pipeline_info,
			nullptr,
			&pipeline._pipeline
		);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create pipeline", VkError(res));
		}

		// Frame attachments
		//TODO: make sure all the images are the same size maybe
		auto frame_images = std::vector<VkImageView>();
		for (auto &attachment : pipeline._render_pass->frame_attachments()) {
			frame_images.push_back(attachment.image_view());
			pipeline._clear_values.push_back(attachment.clear_color());
		}

		auto framebuffer_info = VkFramebufferCreateInfo{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = pipeline._render_pass->render_pass();
		framebuffer_info.attachmentCount = frame_images.size();
		framebuffer_info.pAttachments = frame_images.data();
		framebuffer_info.width = pipeline._render_pass->frame_attachments().front().size().width;
		framebuffer_info.height = pipeline._render_pass->frame_attachments().front().size().height;
		framebuffer_info.layers = 1;

		res = vkCreateFramebuffer(
			Graphics::DEFAULT->device(),
			&framebuffer_info,
			nullptr,
			&pipeline._framebuffer
		);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create framebuffer", VkError(res));
		}

		pipeline._size = {static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)};

		return pipeline;
	}

	Pipeline::Pipeline(Pipeline &&other) {
		_type = other._type;
		_size = other._size;

		_render_pass = util::move_ptr(other._render_pass);

		_pipeline = util::move_ptr(other._pipeline);
		_pipeline_layout = util::move_ptr(other._pipeline_layout);
		_framebuffer = util::move_ptr(other._framebuffer);

		_attachments = std::move(other._attachments);

		_descriptor_set_layouts = std::move(other._descriptor_set_layouts);
	}

	Pipeline &Pipeline::operator=(Pipeline &&other) {
		_type = other._type;
		_size = other._size;

		_render_pass = util::move_ptr(other._render_pass);

		_pipeline = util::move_ptr(other._pipeline);
		_pipeline_layout = util::move_ptr(other._pipeline_layout);
		_framebuffer = util::move_ptr(other._framebuffer);

		_attachments = std::move(other._attachments);

		_descriptor_set_layouts = std::move(other._descriptor_set_layouts);

		return other;
	}

	void Pipeline::destroy() {
		if (_pipeline_layout) {
			vkDestroyPipelineLayout(
				Graphics::DEFAULT->device(),
				_pipeline_layout,
				nullptr
			);
			_pipeline_layout = nullptr;
		}
		if (_pipeline) {
			vkDestroyPipeline(
				Graphics::DEFAULT->device(),
				_pipeline,
				nullptr
			);
			_pipeline = nullptr;
		}
	}

	Pipeline::~Pipeline() {
		destroy();
	}

	VkPipeline Pipeline::pipeline() const {
		return _pipeline;
	}

	VkPipelineLayout Pipeline::pipeline_layout() const {
		return _pipeline_layout;
	}

	std::vector<DescriptorSetLayout> const &Pipeline::layouts() {
		return _descriptor_set_layouts;
	}

	std::vector<VkDescriptorSetLayout> Pipeline::vk_layouts() {
		auto layouts = std::vector<VkDescriptorSetLayout>();
		for (auto &layout : _descriptor_set_layouts) {
			layouts.push_back(layout.layout());
		}
		return layouts;
	}

	VkFramebuffer Pipeline::framebuffer() const {
		return _framebuffer;
	}

	std::vector<VkClearValue> const &Pipeline::clear_values() const {
		return _clear_values;
	}

	Pipeline::Attachments const &Pipeline::attachments() const {
		return _attachments;
	}

	RenderPass const &Pipeline::render_pass() const {
		return *_render_pass;
	}
}
