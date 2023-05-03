#include "mainRenderPipeline.h"
#include "defs.h"
#include "error.h"
#include "file.h"
#include "graphics.h"
#include "log.h"
#include "uniformBufferObject.h"
#include "vulkan/vulkan_core.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <array>
#include <stb_image.h>
#include <format>
#include <sstream>

namespace vulkan {
	MainRenderPipeline::MainRenderPipeline(Graphics const &graphics): graphics_(graphics) {
		createSyncObjects_();
		createCommandBuffers_();
		createSwapchain_();
		createSwapchainImageViews_();
		createRenderPass_();
		createTextureImage_();
		createTextureImageView_();
		createDepthResources_();
		createSwapchainFramebuffers_();
		createDescriptorPool_();
		createDescriptorSetLayout_();
		createUniformBuffers_();
		createDescriptorSets_();
		createPipeline_();
	}

	MainRenderPipeline::~MainRenderPipeline() {
		util::log_memory("Deconstructing main render pipeline");
		
		cleanupSwapchain_();

		vkDestroyPipelineLayout(graphics_.device(), pipelineLayout_, nullptr);
		vkDestroyPipeline(graphics_.device(), pipeline_, nullptr);
		vkDestroyDescriptorSetLayout(graphics_.device(), descriptorSetLayout_, nullptr);
		vkFreeDescriptorSets(graphics_.device(), descriptorPool_, descriptorSets_.size(), descriptorSets_.data());
		vkDestroyDescriptorPool(graphics_.device(), descriptorPool_, nullptr);
		vkDestroyBuffer(graphics_.device(), vertexBuffer_, nullptr);
		vkFreeMemory(graphics_.device(), vertexBufferMemory_, nullptr);
		vkDestroyBuffer(graphics_.device(), indexBuffer_, nullptr);
		vkFreeMemory(graphics_.device(), indexBufferMemory_, nullptr);
		for (auto uniformBuffer : uniformBuffers_) {
			vkDestroyBuffer(graphics_.device(), uniformBuffer, nullptr);
		}
		for (auto uniformBufferMemory : uniformBuffersMemory_) {
			vkFreeMemory(graphics_.device(), uniformBufferMemory, nullptr);
		}
		for (auto inFlightFence : inFlightFences_) {
			vkDestroyFence(graphics_.device(), inFlightFence, nullptr);
		}
		for (auto imageAvailableSemaphore : imageAvailableSemaphores_) {
			vkDestroySemaphore(graphics_.device(), imageAvailableSemaphore, nullptr);
		}
		for (auto renderFinishedSemaphore : renderFinishedSemaphores_) {
			vkDestroySemaphore(graphics_.device(), renderFinishedSemaphore, nullptr);
		}

		vkDestroyImage(graphics_.device(), textureImage_, nullptr);
		vkFreeMemory(graphics_.device(), textureImageMemory_, nullptr);
		vkDestroyImageView(graphics_.device(), textureImageView_, nullptr);


		vkDestroyRenderPass(graphics_.device(), renderPass_, nullptr);
	}

	SwapchainSupportDetails MainRenderPipeline::querySwapchainSupport_(VkPhysicalDevice device, Graphics const &graphics) {
		auto surface = graphics.surface();

		auto details = SwapchainSupportDetails{};

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	void MainRenderPipeline::submit(
			uint32_t frameIndex,
			VkSemaphore previousSemaphore)
	{
		auto submitInfo = VkSubmitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		vkWaitForFences(graphics_.device(), 1, &inFlightFences_[frameIndex], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		auto result = vkAcquireNextImageKHR(
				graphics_.device(),
				swapchain_,
				UINT64_MAX,
				imageAvailableSemaphores_[frameIndex],
				VK_NULL_HANDLE,
				&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapchain_();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw vulkan::Error(result);
		}

		updateUniformBuffer_(frameIndex);

		vkResetFences(graphics_.device(), 1, &inFlightFences_[frameIndex]);

		vkResetCommandBuffer(commandBuffers_[frameIndex], 0);

		//imgui render

		recordCommandBuffer_(commandBuffers_[frameIndex], imageIndex, frameIndex);

		auto waitSemaphores = std::array<VkSemaphore, 2>{previousSemaphore, imageAvailableSemaphores_[frameIndex]};
		auto waitStages = std::array<VkPipelineStageFlags, 2>{VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = waitSemaphores.size();
		submitInfo.pWaitSemaphores = waitSemaphores.data();
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers_[frameIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[frameIndex];

		require(vkQueueSubmit(graphics_.graphicsQueue(), 1, &submitInfo, inFlightFences_[frameIndex]));

		auto presentInfo = VkPresentInfoKHR{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[frameIndex];

		VkSwapchainKHR swapchains[] = {swapchain_};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(graphics_.presentQueue(), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_ ) {
			framebufferResized_ = false;
			recreateSwapchain_();
		} else if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	void MainRenderPipeline::loadVertices(std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
		//TODO: loading vertexes should be abstracted away

		//Load vertex buffer
		auto bufferSize = VkDeviceSize(sizeof(vertices[0]) * vertices.size());

		auto stagingBuffer = VkBuffer{};
		auto stagingBufferMemory = VkDeviceMemory{};
		graphics_.createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory);

		void *data;
		vkMapMemory(graphics_.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t) bufferSize);
		vkUnmapMemory(graphics_.device(), stagingBufferMemory);

		graphics_.createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBuffer_,
				vertexBufferMemory_);

		graphics_.copyBuffer(stagingBuffer, vertexBuffer_, bufferSize);

		vkDestroyBuffer(graphics_.device(), stagingBuffer, nullptr);
		vkFreeMemory(graphics_.device(), stagingBufferMemory, nullptr);

		//Load index buffer
		bufferSize = sizeof(indices[0]) * indices.size();

		graphics_.createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory);

		vkMapMemory(graphics_.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t) bufferSize);
		vkUnmapMemory(graphics_.device(), stagingBufferMemory);

		graphics_.createBuffer(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBuffer_,
				indexBufferMemory_);

		graphics_.copyBuffer(stagingBuffer, indexBuffer_, bufferSize);

		vkDestroyBuffer(graphics_.device(), stagingBuffer, nullptr);
		vkFreeMemory(graphics_.device(), stagingBufferMemory, nullptr);

		indexCount_ = indices.size();
	}

	void MainRenderPipeline::queueResize() {
		framebufferResized_ = true;
	}

	VkExtent2D MainRenderPipeline::swapchainExtent() const {
		return swapchainExtent_;
	}

	void MainRenderPipeline::createSyncObjects_() {
		imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			require(vkCreateSemaphore(graphics_.device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]));
			require(vkCreateSemaphore(graphics_.device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]));
			require(vkCreateFence(graphics_.device(), &fenceInfo, nullptr, &inFlightFences_[i]));
		}
	}

	void MainRenderPipeline::createCommandBuffers_() {
		commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
		auto allocInfo = VkCommandBufferAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = graphics_.commandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers_.size();

		require(vkAllocateCommandBuffers(graphics_.device(), &allocInfo, commandBuffers_.data()));
	}

	void MainRenderPipeline::createSwapchain_() {
		auto swapchainSupport = querySwapchainSupport_(graphics_.physicalDevice(), graphics_);
		auto surfaceFormat = chooseSwapchainSurfaceFormat_(swapchainSupport.formats);
		auto presentMode = chooseSwapchainPresentMode_(swapchainSupport.presentModes);
		auto extent = chooseSwapchainExtent_(swapchainSupport.capabilities);
		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
		if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
			imageCount = swapchainSupport.capabilities.maxImageCount;
		}

		auto createInfo = VkSwapchainCreateInfoKHR{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = graphics_.surface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		auto indices = graphics_.findQueueFamilies();
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		
		createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;
		
		require(vkCreateSwapchainKHR(graphics_.device(), &createInfo, nullptr, &swapchain_));

		vkGetSwapchainImagesKHR(graphics_.device(), swapchain_, &imageCount, nullptr);
		swapchainImages_.resize(imageCount);
		vkGetSwapchainImagesKHR(graphics_.device(), swapchain_, &imageCount, swapchainImages_.data());

		swapchainImageFormat_ = surfaceFormat.format;
		swapchainExtent_ = extent;
	}

	void MainRenderPipeline::createSwapchainImageViews_() {
		swapchainImageViews_.resize(swapchainImages_.size());

		for (uint32_t i = 0; i < swapchainImages_.size(); ++i) {
			swapchainImageViews_[i] = graphics_.createImageView(
					swapchainImages_[i],
					swapchainImageFormat_,
					VK_IMAGE_ASPECT_COLOR_BIT,
					1);
		}
	}

	void MainRenderPipeline::createSwapchainFramebuffers_() {
		swapchainFramebuffers_.resize(swapchainImageViews_.size());

		for (size_t i = 0; i < swapchainImageViews_.size(); ++i) {
			std::array<VkImageView, 2> attachments = {
				swapchainImageViews_[i],
				depthImageView_
			};

			auto framebufferInfo = VkFramebufferCreateInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass_;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapchainExtent_.width;
			framebufferInfo.height = swapchainExtent_.height;
			framebufferInfo.layers = 1;

			require(vkCreateFramebuffer(graphics_.device(), &framebufferInfo, nullptr, &swapchainFramebuffers_[i]));
		}
	}

	void MainRenderPipeline::createRenderPass_() {
		auto colorAttachment = VkAttachmentDescription{};
		colorAttachment.format = swapchainImageFormat_;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		auto colorAttachmentRef = VkAttachmentReference{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 2> attachments = {
			colorAttachment,
			depthAttachment};

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

		require(vkCreateRenderPass(graphics_.device(), &renderPassInfo, nullptr, &renderPass_));
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

		require(vkCreateDescriptorSetLayout(graphics_.device(), &layoutInfo, nullptr, &descriptorSetLayout_));
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

		require(vkCreateDescriptorPool(graphics_.device(), &poolInfo, nullptr, &descriptorPool_));
	}

	void MainRenderPipeline::createUniformBuffers_() {
		auto bufferSize = VkDeviceSize(sizeof(UniformBufferObject));

		uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			graphics_.createBuffer(
					bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					uniformBuffers_[i],
					uniformBuffersMemory_[i]);

			require(vkMapMemory(
						graphics_.device(),
						uniformBuffersMemory_[i],
						0,
						bufferSize,
						0,
						&uniformBuffersMapped_[i]));
		}
	}

	void MainRenderPipeline::createTextureImage_() {
		int texWidth, texHeight, texChannels;
		auto pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		auto imageSize = (VkDeviceSize) texWidth * texHeight * 4;
		mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight))));

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		auto stagingBuffer = VkBuffer{};
		auto stagingBufferMemory = VkDeviceMemory{};
		graphics_.createBuffer(
				imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory);

		void *data;
		vkMapMemory(graphics_.device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(graphics_.device(), stagingBufferMemory);
		stbi_image_free(pixels);

		graphics_.createImage(
				texWidth,
				texHeight,
				mipLevels_,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				textureImage_,
				textureImageMemory_);

		graphics_.transitionImageLayout(
				textureImage_,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				mipLevels_);

		graphics_.copyBufferToImage(
				stagingBuffer,
				textureImage_,
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(graphics_.device(), stagingBuffer, nullptr);
		vkFreeMemory(graphics_.device(), stagingBufferMemory, nullptr);

		graphics_.generateMipmaps(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels_);
	}

	void MainRenderPipeline::createTextureImageView_() {
		textureImageView_ = graphics_.createImageView(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels_);
	}

	void MainRenderPipeline::createDepthResources_() {
		auto depthFormat = findDepthFormat_();
		graphics_.createImage(
				swapchainExtent_.width,
				swapchainExtent_.height,
				1,
				depthFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				depthImage_,
				depthImageMemory_);

		depthImageView_ = graphics_.createImageView(depthImage_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		graphics_.transitionImageLayout(
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
		require(vkAllocateDescriptorSets(graphics_.device(), &allocInfo, descriptorSets_.data()));

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto bufferInfo = VkDescriptorBufferInfo{};
			bufferInfo.buffer = uniformBuffers_[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			auto imageInfo = VkDescriptorImageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView_;
			imageInfo.sampler = graphics_.mainTextureSampler();

			auto computeImageInfo = VkDescriptorImageInfo{};
			computeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			computeImageInfo.imageView = graphics_.computeImageView();
			computeImageInfo.sampler = graphics_.mainTextureSampler();

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
					graphics_.device(),
					static_cast<uint32_t>(descriptorWrites.size()),
					descriptorWrites.data(),
					0,
					nullptr);
		}
	}

	void MainRenderPipeline::createPipeline_() {
		auto vertShaderCode = util::readEnvFile("src/shaders/default_shader.vert.spv");
		auto fragShaderCode = util::readEnvFile("src/shaders/default_shader.frag.spv");

		auto vertShaderModule = graphics_.createShaderModule(vertShaderCode);
		auto fragShaderModule = graphics_.createShaderModule(fragShaderCode);

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
		viewport.width = (float) swapchainExtent_.width;
		viewport.height = (float) swapchainExtent_.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = swapchainExtent_;

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

		require(vkCreatePipelineLayout(graphics_.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout_));

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

		require(vkCreateGraphicsPipelines(graphics_.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_));

		vkDestroyShaderModule(graphics_.device(), fragShaderModule, nullptr);
		vkDestroyShaderModule(graphics_.device(), vertShaderModule, nullptr);
	}

	void MainRenderPipeline::recordCommandBuffer_(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t frameIndex) {
		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		require(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		auto renderPassInfo = VkRenderPassBeginInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass_;
		renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainExtent_;

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
		viewport.width = static_cast<float>(swapchainExtent_.width);
		viewport.height = static_cast<float>(swapchainExtent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = swapchainExtent_;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {vertexBuffer_};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout_,
				0,
				1,
				&descriptorSets_[frameIndex],
				0,
				nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indexCount_), 1, 0, 0, 0);

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
		ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent_.width / (float) swapchainExtent_.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));

	}

	void MainRenderPipeline::recreateSwapchain_() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(graphics_.window(), &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(graphics_.window(), &width, &height);
			glfwWaitEvents();
		}
		graphics_.waitIdle();

		util::log_event("Recreate swap chain");

		cleanupSwapchain_();

		createSwapchain_();
		createSwapchainImageViews_();
		createDepthResources_();
		createSwapchainFramebuffers_();
	}

	void MainRenderPipeline::cleanupSwapchain_() {
		vkDestroyImageView(graphics_.device(), depthImageView_, nullptr);
		vkDestroyImage(graphics_.device(), depthImage_, nullptr);
		vkFreeMemory(graphics_.device(), depthImageMemory_, nullptr);

		for (auto swapchainFramebuffer : swapchainFramebuffers_) {
			vkDestroyFramebuffer(graphics_.device(), swapchainFramebuffer, nullptr);
		}
		for (auto swapchainImageView : swapchainImageViews_) {
			vkDestroyImageView(graphics_.device(), swapchainImageView, nullptr);
		}

		vkDestroySwapchainKHR(graphics_.device(), swapchain_, nullptr);
	}

	VkSurfaceFormatKHR MainRenderPipeline::chooseSwapchainSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
					availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR MainRenderPipeline::chooseSwapchainPresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D MainRenderPipeline::chooseSwapchainExtent_(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(graphics_.window(), &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(
					actualExtent.width,
					capabilities.minImageExtent.width,
					capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(
					actualExtent.height,
					capabilities.minImageExtent.height,
					capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkFormat MainRenderPipeline::findDepthFormat_() {
		return graphics_.findSupportedFormat(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}
