#include "Pipeline.h"
#include "Device.h"
#include "Error.h"
#include "PipelineLayout.h"
#include "RenderPass.h"
#include "ShaderModule.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"
#include <mutex>

namespace vulkan {
	SharedPipeline Pipeline::createShared(
			VkGraphicsPipelineCreateInfo &createInfo,
			SharedDevice device,
			SharedSwapchain swapchain,
			SharedRenderPass renderPass)
	{
		return SharedPipeline(new Pipeline(createInfo, device, swapchain, renderPass));
	}

	SharedPipelineLayout Pipeline::layout() {
		return layout_;
	}

	VkPipeline& Pipeline::operator*() {
		return pipeline_;
	}

	VkPipeline& Pipeline::raw() {
		return pipeline_;
	}

	Pipeline::~Pipeline() {
		vkDestroyPipeline(device_->raw(), pipeline_, nullptr);
	}

	Pipeline::Pipeline(
			VkGraphicsPipelineCreateInfo &createInfo,
			SharedDevice device,
			SharedSwapchain swapchain,
			SharedRenderPass renderPass):
		device_(device),
		swapchain_(swapchain),
		renderPass_(renderPass)
	{
		VkResult result = vkCreateGraphicsPipelines(device_->raw(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	/**** factory ****/

	PipelineFactory::PipelineFactory(
			SharedDevice device,
			SharedSwapchain swapchain,
			SharedRenderPass renderPass):
		device_(device),
		swapchain_(swapchain),
		renderPass_(renderPass)
	{}

	PipelineFactory &PipelineFactory::defaultConfig() {
		vertShaderModule_ = ShaderModule("src/shaders/default_shader.vert.spv", device_);
		fragShaderModule_ = ShaderModule("src/shaders/default_shader.frag.spv", device_);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule_.raw();
		vertShaderStageInfo.pName = "main";
		shaderStages_[0] = vertShaderStageInfo;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule_.raw();
		fragShaderStageInfo.pName = "main";
		shaderStages_[1] = fragShaderStageInfo;

		vertexInputInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo_.vertexBindingDescriptionCount = 0;
		vertexInputInfo_.pVertexBindingDescriptions = nullptr;
		vertexInputInfo_.vertexAttributeDescriptionCount = 0;
		vertexInputInfo_.pVertexAttributeDescriptions = nullptr;

		inputAssembly_.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly_.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly_.primitiveRestartEnable = VK_FALSE;

		viewport_.x = 0.0f;
		viewport_.y = 0.0f;
		viewport_.width = (float) swapchain_->extent().width;
		viewport_.height = (float) swapchain_->extent().height;
		viewport_.minDepth = 0.0f;
		viewport_.maxDepth = 1.0f;

		scissor_.offset = {0, 0};
		scissor_.extent = swapchain_->extent();

		viewportState_.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState_.viewportCount = 1;
		viewportState_.pViewports = &viewport_;
		viewportState_.scissorCount = 1;
		viewportState_.pScissors = &scissor_;

		rasterizer_.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer_.depthClampEnable = VK_FALSE;
		rasterizer_.rasterizerDiscardEnable = VK_FALSE;
		rasterizer_.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer_.lineWidth = 1.0f;
		rasterizer_.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer_.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer_.depthBiasEnable = VK_FALSE;
		rasterizer_.depthBiasConstantFactor = 0.0f;
		rasterizer_.depthBiasClamp = 0.0f;
		rasterizer_.depthBiasSlopeFactor = 0.0f;

		multisampling_.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling_.sampleShadingEnable = VK_FALSE;
		multisampling_.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling_.minSampleShading = 1.0f;
		multisampling_.pSampleMask = nullptr;
		multisampling_.alphaToCoverageEnable = VK_FALSE;
		multisampling_.alphaToOneEnable = VK_FALSE;

		colorBlendAttachment_.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment_.blendEnable = VK_FALSE;
		colorBlendAttachment_.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment_.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment_.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment_.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment_.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment_.alphaBlendOp = VK_BLEND_OP_ADD;

		colorBlending_.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending_.logicOpEnable = VK_FALSE;
		colorBlending_.logicOp = VK_LOGIC_OP_COPY;
		colorBlending_.attachmentCount = 1;
		colorBlending_.pAttachments = &colorBlendAttachment_;
		colorBlending_.blendConstants[0] = 0.0f;
		colorBlending_.blendConstants[1] = 0.0f;
		colorBlending_.blendConstants[2] = 0.0f;
		colorBlending_.blendConstants[3] = 0.0f;

		dynamicStates_ = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		dynamicState_.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState_.dynamicStateCount = static_cast<uint32_t>(dynamicStates_.size());
		dynamicState_.pDynamicStates = dynamicStates_.data();

		layout_ = PipelineLayoutFactory(swapchain_, device_).defaultConfig().createShared();

		createInfo_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		createInfo_.stageCount = 2;
		createInfo_.pStages = shaderStages_;
		createInfo_.pVertexInputState = &vertexInputInfo_;
		createInfo_.pInputAssemblyState = &inputAssembly_;
		createInfo_.pViewportState = &viewportState_;
		createInfo_.pRasterizationState = &rasterizer_;
		createInfo_.pMultisampleState = &multisampling_;
		createInfo_.pDepthStencilState = nullptr;
		createInfo_.pColorBlendState = &colorBlending_;
		createInfo_.pDynamicState = &dynamicState_;
		createInfo_.layout = layout_->raw();
		createInfo_.renderPass = renderPass_->raw();
		createInfo_.subpass = 0;
		createInfo_.basePipelineHandle = VK_NULL_HANDLE;
		createInfo_.basePipelineIndex = -1;

		return *this;
	}

	SharedPipeline PipelineFactory::createShared() {
		return Pipeline::createShared(createInfo_, device_, swapchain_, renderPass_);
	}
}
