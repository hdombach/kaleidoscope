#include "textureMaterial.hpp"
#include "defs.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "mainRenderPipeline.hpp"
#include "materialFactory.hpp"
#include "shader.hpp"
#include "uniformBufferObject.hpp"
#include "vulkan/vulkan_core.h"
#include <functional>
#include <mutex>
#include <vector>

namespace vulkan {
	TextureMaterial::~TextureMaterial() {
		vkDestroyPipelineLayout(Graphics::DEFAULT->device(), pipelineLayout_, nullptr);
		vkDestroyPipeline(Graphics::DEFAULT->device(), pipeline_, nullptr);
		vkDestroyDescriptorSetLayout(
				Graphics::DEFAULT->device(),
				descriptorSetLayout_,
				nullptr);
		vkFreeDescriptorSets(
				Graphics::DEFAULT->device(),
				materialFactory_.descriptorPool(),
				descriptorSets_.size(),
				descriptorSets_.data());
	}
	VkPipelineLayout TextureMaterial::pipelineLayout() const {
		return pipelineLayout_;
	}

	VkPipeline TextureMaterial::pipeline() const {
		return pipeline_;
	}

	std::vector<VkDescriptorSet> TextureMaterial::getDescriptorSet(uint32_t frameIndex) const {
		return std::vector<VkDescriptorSet>{
			descriptorSets_[frameIndex]
		};
	}

	TextureMaterial::TextureMaterial(
			MaterialFactory const &materialFactory,
			Texture const *texture):
		texture_(texture),
		materialFactory_(materialFactory)
	{
		createDescriptorSetLayout_();
		createDescriptorSets_(materialFactory);
		createPipeline_(materialFactory);
	}

	void TextureMaterial::createDescriptorSetLayout_() {
		auto uboLayoutBinding = VkDescriptorSetLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		auto samplerLayoutBinding = VkDescriptorSetLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		auto bindings = std::array<VkDescriptorSetLayoutBinding, 2>{uboLayoutBinding, samplerLayoutBinding};
		auto layoutInfo = VkDescriptorSetLayoutCreateInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		require(vkCreateDescriptorSetLayout(Graphics::DEFAULT->device(), &layoutInfo, nullptr, &descriptorSetLayout_));
	}

	void TextureMaterial::createDescriptorSets_(MaterialFactory const &materialFactory) {
		auto layouts = std::vector<VkDescriptorSetLayout>(FRAMES_IN_FLIGHT, descriptorSetLayout_);
		auto allocInfo = VkDescriptorSetAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = materialFactory_.descriptorPool();
		allocInfo.descriptorSetCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets_.resize(FRAMES_IN_FLIGHT);
		require(vkAllocateDescriptorSets(Graphics::DEFAULT->device(), &allocInfo, descriptorSets_.data()));

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto bufferInfo = VkDescriptorBufferInfo{};
			bufferInfo.buffer = materialFactory.mainRenderPipeline().uniformBuffers()[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			auto imageInfo = VkDescriptorImageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = texture_->imageView();
			imageInfo.sampler = Graphics::DEFAULT->mainTextureSampler();

			auto descriptorWrites = std::array<VkWriteDescriptorSet, 2>{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets_[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets_[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(
					Graphics::DEFAULT->device(), 
					static_cast<uint32_t>(descriptorWrites.size()), 
					descriptorWrites.data(), 
					0, 
					nullptr);
		}
	}

	void TextureMaterial::createPipeline_(MaterialFactory const &materialFactory) {
		auto vertShader = vulkan::Shader::fromEnvFile("src/shaders/default_shader.vert.spv");
		auto fragShader = vulkan::Shader::fromEnvFile("src/shaders/default_shader.frag.spv");

		auto shaderStages = std::array<VkPipelineShaderStageCreateInfo, 2>();

		/* vert shader stage */
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = vertShader.shaderModule();
		shaderStages[0].pName = "main";

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = fragShader.shaderModule();
		shaderStages[1].pName = "main";

		auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{};

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.width = (float) 100;
		viewport.height = (float) 100;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = VkExtent2D{100, 100};

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

		auto dynamicStates = std::array<VkDynamicState, 2>{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		auto dynamicState = VkPipelineDynamicStateCreateInfo{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		//uniforms
		auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		require(vkCreatePipelineLayout(Graphics::DEFAULT->device(), &pipelineLayoutInfo, nullptr, &pipelineLayout_));

		auto depthStencil = VkPipelineDepthStencilStateCreateInfo{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		auto pipelineInfo = VkGraphicsPipelineCreateInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout_;
		pipelineInfo.renderPass = materialFactory.mainRenderPipeline().renderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		require(vkCreateGraphicsPipelines(Graphics::DEFAULT->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_));
	}
}
