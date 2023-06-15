#include "mainRenderPipeline.h"
#include "defs.h"
#include "error.h"
#include "file.h"
#include "format.h"
#include "graphics.h"
#include "imgui_impl_vulkan.h"
#include "log.h"
#include "../ui/window.h"
#include "uniformBufferObject.h"
#include "vulkan/vulkan_core.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <glm/fwd.hpp>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <array>
#include <stb_image.h>
#include <format>
#include <sstream>

namespace vulkan {
	MainRenderPipeline::MainRenderPipeline(
			types::ResourceManager &resourceManager,
			VkExtent2D size):
		size_(size),
		resourceManager_(resourceManager)
	{
		createSyncObjects_();
		createCommandBuffers_();
		createRenderPass_();
		createDepthResources_();
		createResultImages_();
		createDescriptorPool_();
		createDescriptorSetLayout_();
		createUniformBuffers_();
		createDescriptorSets_();
		createPipeline_();
	}

	MainRenderPipeline::~MainRenderPipeline() {
		util::log_memory("Deconstructing main render pipeline");

		cleanupDepthResources_();
		cleanupResultImages_();
		
		vkDestroyPipelineLayout(Graphics::DEFAULT->device(), pipelineLayout_, nullptr);
		vkDestroyPipeline(Graphics::DEFAULT->device(), pipeline_, nullptr);
		vkDestroyDescriptorSetLayout(Graphics::DEFAULT->device(), descriptorSetLayout_, nullptr);
		vkFreeDescriptorSets(Graphics::DEFAULT->device(), descriptorPool_, descriptorSets_.size(), descriptorSets_.data());
		vkDestroyDescriptorPool(Graphics::DEFAULT->device(), descriptorPool_, nullptr);
		for (auto uniformBuffer : uniformBuffers_) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), uniformBuffer, nullptr);
		}
		for (auto uniformBufferMemory : uniformBuffersMemory_) {
			vkFreeMemory(Graphics::DEFAULT->device(), uniformBufferMemory, nullptr);
		}
		for (auto inFlightFence : inFlightFences_) {
			vkDestroyFence(Graphics::DEFAULT->device(), inFlightFence, nullptr);
		}
		for (auto renderFinishedSemaphore : renderFinishedSemaphores_) {
			vkDestroySemaphore(Graphics::DEFAULT->device(), renderFinishedSemaphore, nullptr);
		}

		vkDestroyRenderPass(Graphics::DEFAULT->device(), renderPass_, nullptr);
	}

	void MainRenderPipeline::submit() {
		vkWaitForFences(Graphics::DEFAULT->device(), 1, &inFlightFences_[frameIndex_], VK_TRUE, UINT64_MAX);
		if (framebufferResized_ ) {
			framebufferResized_ = false;
			recreateResultImages_();
		}
		auto submitInfo = VkSubmitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		updateUniformBuffer_(frameIndex_);

		vkResetFences(Graphics::DEFAULT->device(), 1, &inFlightFences_[frameIndex_]);

		vkResetCommandBuffer(commandBuffers_[frameIndex_], 0);

		recordCommandBuffer_(commandBuffers_[frameIndex_]);

		auto waitStages = std::array<VkPipelineStageFlags, 2>{VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers_[frameIndex_];
		//submitInfo.signalSemaphoreCount = 1;
		//submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[frameIndex_];

		require(vkQueueSubmit(Graphics::DEFAULT->graphicsQueue(), 1, &submitInfo, inFlightFences_[frameIndex_]));

		frameIndex_ = (frameIndex_ + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void MainRenderPipeline::resize(glm::ivec2 size) {
		framebufferResized_ = true;
		this->size_ = VkExtent2D{static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y)};
	}
	bool MainRenderPipeline::isResizable() const {
		return true;
	}

	VkExtent2D MainRenderPipeline::getSize() const {
		return size_;
	}

	VkDescriptorSet MainRenderPipeline::getDescriptorSet() const {
		return resultDescriptorSets_[frameIndex_];
	}

	VkImageView MainRenderPipeline::imageView() const {
		return resultImageViews_[frameIndex_];
	}

	void MainRenderPipeline::createSyncObjects_() {
		renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			require(vkCreateSemaphore(Graphics::DEFAULT->device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]));
			require(vkCreateFence(Graphics::DEFAULT->device(), &fenceInfo, nullptr, &inFlightFences_[i]));
		}
	}

	void MainRenderPipeline::createCommandBuffers_() {
		commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
		auto allocInfo = VkCommandBufferAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = Graphics::DEFAULT->commandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers_.size();

		require(vkAllocateCommandBuffers(Graphics::DEFAULT->device(), &allocInfo, commandBuffers_.data()));
	}


	void MainRenderPipeline::createRenderPass_() {
		auto resultAttachment = VkAttachmentDescription{};
		resultAttachment.format = RESULT_IMAGE_FORMAT_;
		resultAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		resultAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		resultAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		resultAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		resultAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		resultAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		resultAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		auto resultAttachmentRef = VkAttachmentReference{};
		resultAttachmentRef.attachment = 0;
		resultAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto depthAttachment = VkAttachmentDescription{};
		depthAttachment.format = findDepthFormat_();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		auto depthAttachmentRef = VkAttachmentReference{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &resultAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 2> attachments = {
			resultAttachment,
			depthAttachment,
		};

		auto renderPassInfo = VkRenderPassCreateInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		auto dependency = VkSubpassDependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		require(vkCreateRenderPass(Graphics::DEFAULT->device(), &renderPassInfo, nullptr, &renderPass_));
	}

	void MainRenderPipeline::createDescriptorSetLayout_() {
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

		auto computeSamperLayoutBinding = VkDescriptorSetLayoutBinding{};
		computeSamperLayoutBinding.binding = 2;
		computeSamperLayoutBinding.descriptorCount = 1;
		computeSamperLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		computeSamperLayoutBinding.pImmutableSamplers = nullptr;
		computeSamperLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		auto bindings = std::array<VkDescriptorSetLayoutBinding, 3>{uboLayoutBinding, samplerLayoutBinding, computeSamperLayoutBinding};
		auto layoutInfo = VkDescriptorSetLayoutCreateInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		require(vkCreateDescriptorSetLayout(Graphics::DEFAULT->device(), &layoutInfo, nullptr, &descriptorSetLayout_));
	}

	void MainRenderPipeline::createDescriptorPool_() {
		auto poolSizes = std::array<VkDescriptorPoolSize, 3>{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		auto poolInfo = VkDescriptorPoolCreateInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		require(vkCreateDescriptorPool(Graphics::DEFAULT->device(), &poolInfo, nullptr, &descriptorPool_));
	}

	void MainRenderPipeline::createUniformBuffers_() {
		auto bufferSize = VkDeviceSize(sizeof(UniformBufferObject));

		uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			Graphics::DEFAULT->createBuffer(
					bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					uniformBuffers_[i],
					uniformBuffersMemory_[i]);

			require(vkMapMemory(
						Graphics::DEFAULT->device(),
						uniformBuffersMemory_[i],
						0,
						bufferSize,
						0,
						&uniformBuffersMapped_[i]));
		}
	}

	void MainRenderPipeline::createDepthResources_() {
		auto depthFormat = findDepthFormat_();
		Graphics::DEFAULT->createImage(
				size_.width,
				size_.height,
				1,
				depthFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				depthImage_,
				depthImageMemory_);

		depthImageView_ = Graphics::DEFAULT->createImageView(depthImage_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		Graphics::DEFAULT->transitionImageLayout(
				depthImage_,
				depthFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				1);
	}

	void MainRenderPipeline::createDescriptorSets_() {
		auto layouts = std::vector<VkDescriptorSetLayout>(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_);
		auto allocInfo = VkDescriptorSetAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool_;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
		require(vkAllocateDescriptorSets(Graphics::DEFAULT->device(), &allocInfo, descriptorSets_.data()));

		auto mainTextureView = resourceManager_.getTexture("viking_room");

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto bufferInfo = VkDescriptorBufferInfo{};
			bufferInfo.buffer = uniformBuffers_[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			auto imageInfo = VkDescriptorImageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = mainTextureView->imageView();
			imageInfo.sampler = Graphics::DEFAULT->mainTextureSampler();

			auto computeImageInfo = VkDescriptorImageInfo{};
			computeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			computeImageInfo.imageView = Graphics::DEFAULT->computeImageView();
			computeImageInfo.sampler = Graphics::DEFAULT->mainTextureSampler();

			auto descriptorWrites = std::array<VkWriteDescriptorSet, 3>{};

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

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets_[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &computeImageInfo;

			vkUpdateDescriptorSets(
					Graphics::DEFAULT->device(),
					static_cast<uint32_t>(descriptorWrites.size()),
					descriptorWrites.data(),
					0,
					nullptr);
		}
	}

	void MainRenderPipeline::createPipeline_() {
		auto vertShaderCode = util::readEnvFile("src/shaders/default_shader.vert.spv");
		auto fragShaderCode = util::readEnvFile("src/shaders/default_shader.frag.spv");

		auto vertShaderModule = Graphics::DEFAULT->createShaderModule(vertShaderCode);
		auto fragShaderModule = Graphics::DEFAULT->createShaderModule(fragShaderCode);

		auto vertShaderStageInfo = VkPipelineShaderStageCreateInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		auto fragShaderStageInfo = VkPipelineShaderStageCreateInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

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
		viewport.y = 0.0f;
		viewport.width = (float) size_.width;
		viewport.height = (float) size_.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = size_;

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

		std::vector<VkDynamicState> dynamicStates {
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
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout_;
		pipelineInfo.renderPass = renderPass_;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		require(vkCreateGraphicsPipelines(Graphics::DEFAULT->device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_));

		vkDestroyShaderModule(Graphics::DEFAULT->device(), fragShaderModule, nullptr);
		vkDestroyShaderModule(Graphics::DEFAULT->device(), vertShaderModule, nullptr);
	}

	void MainRenderPipeline::createResultImages_() {
		resultImages_.resize(Graphics::MIN_IMAGE_COUNT);
		resultImageViews_.resize(Graphics::MIN_IMAGE_COUNT);
		resultImageMemory_.resize(Graphics::MIN_IMAGE_COUNT);
		resultImageFramebuffer_.resize(Graphics::MIN_IMAGE_COUNT);
		resultDescriptorSets_.resize(Graphics::MIN_IMAGE_COUNT);

		for (int i = 0; i < Graphics::MIN_IMAGE_COUNT; i++) {
			Graphics::DEFAULT->createImage(
					size_.width,
					size_.height,
					1,
					RESULT_IMAGE_FORMAT_,
					VK_IMAGE_TILING_OPTIMAL,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					resultImages_[i],
					resultImageMemory_[i]);
			resultImageViews_[i] = Graphics::DEFAULT->createImageView(
					resultImages_[i], 
					RESULT_IMAGE_FORMAT_, 
					VK_IMAGE_ASPECT_COLOR_BIT, 
					1);
			Graphics::DEFAULT->transitionImageLayout(
					resultImages_[i], 
					RESULT_IMAGE_FORMAT_, 
					VK_IMAGE_LAYOUT_UNDEFINED, 
					VK_IMAGE_LAYOUT_GENERAL, 
					1);

			auto attachments = std::array<VkImageView, 2>{
				resultImageViews_[i],
				depthImageView_,
			};

			auto framebufferInfo = VkFramebufferCreateInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass_;
			framebufferInfo.attachmentCount = attachments.size();
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = size_.width;
			framebufferInfo.height = size_.height;
			framebufferInfo.layers = 1;

			require(vkCreateFramebuffer(
						Graphics::DEFAULT->device(),
						&framebufferInfo,
						nullptr,
						&resultImageFramebuffer_[i]));

			if (resultDescriptorSets_[i]) {
				ImGui_ImplVulkan_RemoveTexture(resultDescriptorSets_[i]);
			}
			resultDescriptorSets_[i] = ImGui_ImplVulkan_AddTexture(
					Graphics::DEFAULT->mainTextureSampler(),
					resultImageViews_[i],
					VK_IMAGE_LAYOUT_GENERAL);
		}
	}

	void MainRenderPipeline::recreateResultImages_() {
		Graphics::DEFAULT->waitIdle();
		cleanupDepthResources_();
		cleanupResultImages_();
		createDepthResources_();
		createResultImages_();
	}

	void MainRenderPipeline::cleanupResultImages_() {
		for (auto imageView : resultImageViews_) {
			vkDestroyImageView(Graphics::DEFAULT->device(), imageView, nullptr);
		}
		for (auto image : resultImages_) {
			vkDestroyImage(Graphics::DEFAULT->device(), image, nullptr);
		}
		for (auto memory : resultImageMemory_) {
			vkFreeMemory(Graphics::DEFAULT->device(), memory, nullptr);
		}
		for (auto framebuffer : resultImageFramebuffer_) {
			vkDestroyFramebuffer(Graphics::DEFAULT->device(), framebuffer, nullptr);
		}
	}

	void MainRenderPipeline::cleanupDepthResources_() {
		vkDestroyImage(Graphics::DEFAULT->device(), depthImage_, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), depthImageMemory_, nullptr);
		vkDestroyImageView(Graphics::DEFAULT->device(), depthImageView_, nullptr);
	}

	void MainRenderPipeline::recordCommandBuffer_(VkCommandBuffer commandBuffer) {
		auto mainMesh = resourceManager_.getMesh("viking_room");

		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		require(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		auto renderPassInfo = VkRenderPassBeginInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass_;
		renderPassInfo.framebuffer = resultImageFramebuffer_[frameIndex_];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = size_;

		auto clearValues = std::array<VkClearValue, 2>{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(size_.width);
		viewport.height = static_cast<float>(size_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = size_;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {mainMesh->vertexBuffer()};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, mainMesh->indexBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout_,
				0,
				1,
				&descriptorSets_[frameIndex_],
				0,
				nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mainMesh->indexCount()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);
		require(vkEndCommandBuffer(commandBuffer));
	}

	void MainRenderPipeline::updateUniformBuffer_(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), size_.width / (float) size_.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));

	}

	VkFormat MainRenderPipeline::findDepthFormat_() {
		return Graphics::DEFAULT->findSupportedFormat(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}
