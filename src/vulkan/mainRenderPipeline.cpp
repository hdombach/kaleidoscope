#include <cstdint>
#include <cstdio>
#include <cstring>
#include <array>

#include "mainRenderPipeline.hpp"
#include "defs.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "imgui_impl_vulkan.h"
#include "../util/log.hpp"
#include "semaphore.hpp"
#include "uniformBufferObject.hpp"
#include "imageView.hpp"

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <glm/fwd.hpp>
#include <stb_image.h>

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
		createUniformBuffers_();
	}

	MainRenderPipeline::~MainRenderPipeline() {
		util::log_memory("Deconstructing main render pipeline");

		cleanupDepthResources_();
		cleanupResultImages_();
		
		for (auto uniformBuffer : uniformBuffers_) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), uniformBuffer, nullptr);
		}
		for (auto uniformBufferMemory : uniformBuffersMemory_) {
			vkFreeMemory(Graphics::DEFAULT->device(), uniformBufferMemory, nullptr);
		}
		inFlightFences_.clear();
		renderFinishedSemaphores_.clear();

		vkDestroyRenderPass(Graphics::DEFAULT->device(), renderPass_, nullptr);
	}

	void MainRenderPipeline::submit() {
		require(inFlightFences_[frameIndex_].wait());
		if (framebufferResized_ ) {
			framebufferResized_ = false;
			recreateResultImages_();
		}
		auto submitInfo = VkSubmitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		updateUniformBuffer_(frameIndex_);

		inFlightFences_[frameIndex_].reset();

		vkResetCommandBuffer(commandBuffers_[frameIndex_], 0);

		recordCommandBuffer_(commandBuffers_[frameIndex_]);

		auto waitStages = std::array<VkPipelineStageFlags, 2>{VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers_[frameIndex_];
		//submitInfo.signalSemaphoreCount = 1;
		//submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[frameIndex_];

		require(vkQueueSubmit(Graphics::DEFAULT->graphicsQueue(), 1, &submitInfo, *inFlightFences_[frameIndex_]));

		frameIndex_ = (frameIndex_ + 1) % FRAMES_IN_FLIGHT;
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

	ImageView const &MainRenderPipeline::imageView() const {
		return _resultImageViews[frameIndex_];
	}
	VkRenderPass MainRenderPipeline::renderPass() const {
		return renderPass_;
	}
	std::vector<VkBuffer> const &MainRenderPipeline::uniformBuffers() const {
		return uniformBuffers_;
	}

	util::Result<void, KError> MainRenderPipeline::createSyncObjects_() {
		renderFinishedSemaphores_.resize(FRAMES_IN_FLIGHT);
		inFlightFences_.resize(FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto semaphore = Semaphore::create();
			RETURN_IF_ERR(semaphore);
			renderFinishedSemaphores_[i] = std::move(semaphore.value());

			auto fence = Fence::create();
			RETURN_IF_ERR(fence);
			inFlightFences_[i] = std::move(fence.value());
		}
		return {};
	}

	void MainRenderPipeline::createCommandBuffers_() {
		commandBuffers_.resize(FRAMES_IN_FLIGHT);
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

	void MainRenderPipeline::createUniformBuffers_() {
		auto bufferSize = VkDeviceSize(sizeof(UniformBufferObject));

		uniformBuffers_.resize(FRAMES_IN_FLIGHT);
		uniformBuffersMemory_.resize(FRAMES_IN_FLIGHT);
		uniformBuffersMapped_.resize(FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
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

	util::Result<void, KError> MainRenderPipeline::createResultImages_() {
		resultImages_.resize(FRAMES_IN_FLIGHT);
		_resultImageViews.resize(FRAMES_IN_FLIGHT);
		resultImageMemory_.resize(FRAMES_IN_FLIGHT);
		resultImageFramebuffer_.resize(FRAMES_IN_FLIGHT);
		resultDescriptorSets_.resize(FRAMES_IN_FLIGHT);

		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
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

			auto image_view_info = ImageView::create_info(resultImages_[i]);
			image_view_info.format = RESULT_IMAGE_FORMAT_;
			auto image_view = ImageView::create(image_view_info);
			RETURN_IF_ERR(image_view);
			_resultImageViews[i] = std::move(image_view.value());

			Graphics::DEFAULT->transitionImageLayout(
					resultImages_[i], 
					RESULT_IMAGE_FORMAT_, 
					VK_IMAGE_LAYOUT_UNDEFINED, 
					VK_IMAGE_LAYOUT_GENERAL, 
					1);

			auto attachments = std::array<VkImageView, 2>{
				_resultImageViews[i].value(),
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
					_resultImageViews[i].value(),
					VK_IMAGE_LAYOUT_GENERAL);
		}

		return {};
	}

	void MainRenderPipeline::recreateResultImages_() {
		Graphics::DEFAULT->waitIdle();
		cleanupDepthResources_();
		cleanupResultImages_();
		createDepthResources_();
		createResultImages_();
	}

	void MainRenderPipeline::cleanupResultImages_() {
		_resultImageViews.clear();
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
		auto mainMaterial = resourceManager_.getMaterial("viking_room");

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mainMaterial->pipeline());

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

		auto descriptorSets = mainMaterial->getDescriptorSet(frameIndex_);

		vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				mainMaterial->pipelineLayout(),
				0,
				descriptorSets.size(),
				descriptorSets.data(),
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
