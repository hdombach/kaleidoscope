#include "Graphics.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DebugUtilsMessenger.h"
#include "Defs.h"
#include "Device.h"
#include "Error.h"
#include "Fence.h"
#include "Framebuffer.h"
#include "Instance.h"
#include "PhysicalDevice.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Semaphore.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Window.h"
#include "log.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan.h>

namespace vulkan {
	Graphics::Graphics(SharedWindow window) {
		window_ = window;
		window_->set_graphics(this);
		instance_ = std::make_shared<Instance>(Instance("Kaleidoscope"));
		if (ENABLE_VALIDATION_LAYERS) {
			debugMessenger_ = std::make_shared<DebugUtilsMessenger>(DebugUtilsMessenger(instance_));
		}
		surface_ = std::make_shared<Surface>(instance_, window_);
		physicalDevice_ = PhysicalDevice::pickDevice(surface_, instance_);
		device_ = std::make_shared<Device>(physicalDevice_);
		swapchain_ = std::make_shared<Swapchain>(surface_, device_, window_);
		renderPass_ = std::make_shared<RenderPass>(device_, swapchain_);
		pipeline_ = std::make_shared<Pipeline>(device_, swapchain_, renderPass_);
		for (auto imageView : swapchain_->imageViews()) {
			swapChainFramebuffers_.push_back(std::make_shared<Framebuffer>(imageView, swapchain_, renderPass_, device_));
		}
		commandPool_ = std::make_shared<CommandPool>(device_, physicalDevice_);
		commandBuffers_.reserve(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			commandBuffers_.push_back(std::make_shared<CommandBuffer>(device_, commandPool_));
		}
		/* create sync objects */
		imageAvailableSemaphores_.reserve(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			imageAvailableSemaphores_.push_back(std::make_shared<Semaphore>(device_));
		}
		renderFinishedSemaphores_.reserve(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			renderFinishedSemaphores_.push_back(std::make_shared<Semaphore>(device_));
		}
		inFlightFences_.reserve(MAX_FRAMES_IN_FLIGHT);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			inFlightFences_.push_back(std::make_shared<Fence>(device_));
		}
		framebufferResized_ = false;
	}

	void Graphics::drawFrame() {
		vkWaitForFences(device_->raw(), 1, &inFlightFences_[currentFrame_]->raw(), VK_TRUE, UINT64_MAX);
		vkResetFences(device_->raw(), 1, &inFlightFences_[currentFrame_]->raw());

		uint32_t imageIndex;
		auto result = vkAcquireNextImageKHR(device_->raw(), swapchain_->raw(), UINT64_MAX, imageAvailableSemaphores_[currentFrame_]->raw(), VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			util::log_event("Out of date KHR");
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw vulkan::Error(result);
		}

		vkResetFences(device_->raw(), 1, &inFlightFences_[currentFrame_]->raw());

		vkResetCommandBuffer(commandBuffers_[currentFrame_]->raw(), 0);

		recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex);

		auto submitInfo = VkSubmitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphores_[currentFrame_]->raw();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_]->raw();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[currentFrame_]->raw();

		result = vkQueueSubmit(device_->graphicsQueue(), 1, &submitInfo, inFlightFences_[currentFrame_]->raw());
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		auto presentInfo = VkPresentInfoKHR{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_]->raw();

		VkSwapchainKHR swapChains[] = {swapchain_->raw()};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(device_->presentQueue(), &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
			framebufferResized_ = false;
			util::log_event(std::string("Out of date KHR 2 ") + std::string(string_VkResult(result)));
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Graphics::waitIdle() {
		device_->waitIdle();
	}

	void Graphics::recreateSwapChain() {
		util::log_event("recreate swapchain");
		int width = 0, height = 0;
		glfwGetFramebufferSize(window_->raw(), &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window_->raw(), &width, &height);
			glfwWaitEvents();
		}

		waitIdle();

		swapchain_->reset();
		swapChainFramebuffers_ = {};

		swapchain_ = std::make_shared<Swapchain>(surface_, device_, window_);
		for (auto imageView : swapchain_->imageViews()) {
			swapChainFramebuffers_.push_back(std::make_shared<Framebuffer>(imageView, swapchain_, renderPass_, device_));
		}
	}

	void Graphics::triggerResize() {
		framebufferResized_ = true;
	}

	void Graphics::recordCommandBuffer(SharedCommandBuffer commandBuffer, uint32_t imageIndex) {
		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		auto result = vkBeginCommandBuffer(commandBuffer->raw(), &beginInfo);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		auto renderPassInfo = VkRenderPassBeginInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass_->raw();
		renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex]->raw();
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchain_->extent();

		auto clearColor = VkClearValue{{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers_[currentFrame_]->raw(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers_[currentFrame_]->raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->raw());

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain_->extent().width);
		viewport.height = static_cast<float>(swapchain_->extent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers_[currentFrame_]->raw(), 0, 1, &viewport);

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = swapchain_->extent();
		vkCmdSetScissor(commandBuffers_[currentFrame_]->raw(), 0, 1, &scissor);

		vkCmdDraw(commandBuffers_[currentFrame_]->raw(), 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers_[currentFrame_]->raw());

		result = vkEndCommandBuffer(commandBuffers_[currentFrame_]->raw());
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}
}
