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
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vulkan/vulkan.h>

namespace vulkan {
	Graphics::Graphics(SharedWindow window) {
		window_ = window;
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
		commandBuffer_ = std::make_shared<CommandBuffer>(device_, commandPool_);
		/* create sync objects */
		imageAvailableSemaphore_ = std::make_shared<Semaphore>(device_);
		renderFinishedSemaphore_ = std::make_shared<Semaphore>(device_);
		inFlightFence_ = std::make_shared<Fence>(device_);
	}

	void Graphics::drawFrame() {
		vkWaitForFences(device_->raw(), 1, &inFlightFence_->raw(), VK_TRUE, UINT64_MAX);
		vkResetFences(device_->raw(), 1, &inFlightFence_->raw());

		uint32_t imageIndex;
		vkAcquireNextImageKHR(device_->raw(), swapchain_->raw(), UINT64_MAX, imageAvailableSemaphore_->raw(), VK_NULL_HANDLE, &imageIndex);

		vkResetCommandBuffer(commandBuffer_->raw(), 0);

		recordCommandBuffer(commandBuffer_, imageIndex);

		auto submitInfo = VkSubmitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAvailableSemaphore_->raw();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer_->raw();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphore_->raw();

		auto result = vkQueueSubmit(device_->graphicsQueue(), 1, &submitInfo, inFlightFence_->raw());
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		auto presentInfo = VkPresentInfoKHR{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphore_->raw();

		VkSwapchainKHR swapChains[] = {swapchain_->raw()};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		vkQueuePresentKHR(device_->presentQueue(), &presentInfo);
	}

	void Graphics::waitIdle() {
		device_->waitIdle();
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

		vkCmdBeginRenderPass(commandBuffer_->raw(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer_->raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->raw());

		auto viewport = VkViewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain_->extent().width);
		viewport.height = static_cast<float>(swapchain_->extent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer_->raw(), 0, 1, &viewport);

		auto scissor = VkRect2D{};
		scissor.offset = {0, 0};
		scissor.extent = swapchain_->extent();
		vkCmdSetScissor(commandBuffer_->raw(), 0, 1, &scissor);

		vkCmdDraw(commandBuffer_->raw(), 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer_->raw());

		result = vkEndCommandBuffer(commandBuffer_->raw());
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}
}
