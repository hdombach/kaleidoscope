#include "Pipeline.h"
#include "Device.h"
#include "Error.h"
#include "PipelineLayout.h"
#include "RenderPass.h"
#include "ShaderModule.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	void PipelineDeleter::operator()(PipelineData *data) const {
		vkDestroyPipeline(data->device_->raw(), data->pipeline_, nullptr);
		delete data;
	}

	Pipeline::Pipeline(SharedDevice device, SharedSwapchain swapchain, SharedRenderPass renderPass):
		base_type(new PipelineData{nullptr})
	{
		auto vertShaderModule = ShaderModule("src/shaders/default_shader.vert.spv", device);
		auto fragShaderModule = ShaderModule("src/shaders/default_shader.frag.spv", device);

		VkPipelineShaderStageCreateInfo shaderStages[2];

		auto vertShaderStageInfo = VkPipelineShaderStageCreateInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule.raw();
		vertShaderStageInfo.pName = "main";
		shaderStages[0] = vertShaderStageInfo;

		auto fragShaderStageInfo = VkPipelineShaderStageCreateInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule.raw();
		fragShaderStageInfo.pName = "main";
		shaderStages[1] = fragShaderStageInfo;

		auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapchain->extent().width;
		viewport.height = (float) swapchain->extent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = swapchain->extent();

		auto viewportState = VkPipelineViewportStateCreateInfo{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		auto rasterizer = VkPipelineRasterizationStateCreateInfo{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

		auto colorBlendAttachment = VkPipelineColorBlendAttachmentState{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		auto colorBlending = VkPipelineColorBlendStateCreateInfo{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		auto dynamicStates = std::vector<VkDynamicState>{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		auto dynamicState = VkPipelineDynamicStateCreateInfo{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		auto layout = PipelineLayoutFactory(swapchain, device).defaultConfig().createShared();

		auto createInfo = VkGraphicsPipelineCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo.stageCount = 2;
		createInfo.pStages = shaderStages;
		createInfo.pVertexInputState = &vertexInputInfo;
		createInfo.pInputAssemblyState = &inputAssembly;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizer;
		createInfo.pMultisampleState = &multisampling;
		createInfo.pDepthStencilState = nullptr;
		createInfo.pColorBlendState = &colorBlending;
		createInfo.pDynamicState = &dynamicState;
		createInfo.layout = layout->raw();
		createInfo.renderPass = renderPass->raw();
		createInfo.subpass = 0;
		createInfo.basePipelineHandle = VK_NULL_HANDLE;
		createInfo.basePipelineIndex = -1;

		{
			auto data = get();
			VkResult result = vkCreateGraphicsPipelines(device->raw(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &data->pipeline_);
			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}

			data->device_ = device;
			data->swapchain_ = swapchain;
			data->renderPass_ = renderPass;
			data->layout_ = layout;
		}
	}

	VkPipeline& Pipeline::raw() {
		return get()->pipeline_;
	}

	SharedPipelineLayout Pipeline::layout() {
		return get()->layout_;
	}
}
