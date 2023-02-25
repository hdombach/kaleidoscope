#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DebugUtilsMessenger.h"
#include "Defs.h"
#include "Device.h"
#include "Instance.h"
#include "PhysicalDevice.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Semaphore.h"
#include "ShaderModule.h"
#include "Fence.h"
#include "Surface.h"
#include "Swapchain.h"
#include "Window.h"
#include "Framebuffer.h"
#include "util/file.h"
#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>
#define GLFW_INCLUDE_VULKAN

#include <exception>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <fstream>

//Queue family things
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class KaleidoscopeApplication {
	public:
		void run();

	private:
		void initVulkan();
		void recordCommandBuffer(vulkan::SharedCommandBuffer commandBuffer, uint32_t imageIndex);

		void drawFrame();

		void mainLoop();
		void cleanup();
		VkShaderModule createShaderModule(const std::vector<char>& code);

		//callbacks
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
				void* pUserData);

		const uint32_t WIDTH = 800;
		const uint32_t HEIGHT = 600;
		vulkan::SharedWindow window;
		vulkan::SharedInstance instance;
		vulkan::SharedDebugUtilsMessenger debugMessenger;
		vulkan::PhysicalDevice physicalDevice;
		vulkan::SharedDevice device;
		vulkan::SharedSurface surface;
		vulkan::SharedSwapchain swapchain;
		vulkan::SharedRenderPass renderPass;
		vulkan::SharedPipeline pipeline;
		std::vector<vulkan::SharedFramebuffer> swapChainFramebuffers;
		vulkan::SharedCommandPool commandPool;
		vulkan::SharedCommandBuffer commandBuffer;
		vulkan::SharedSemaphore imageAvailableSemaphore;
		vulkan::SharedSemaphore renderFinishedSemaphore;
		vulkan::SharedFence inFlightFence;
};

void KaleidoscopeApplication::run() {
	glfwInit();
	window = std::make_shared<vulkan::Window>(vulkan::Window("Kaleidoscope"));
	initVulkan();
	mainLoop();
	cleanup();
}

void KaleidoscopeApplication::initVulkan() {
	instance = std::make_shared<vulkan::Instance>(vulkan::Instance("Kaleidoscope"));
	if (vulkan::ENABLE_VALIDATION_LAYERS) {
		debugMessenger = std::make_shared<vulkan::DebugUtilsMessenger>(vulkan::DebugUtilsMessenger(instance));
	}
	surface = std::make_shared<vulkan::Surface>(instance, window);
	physicalDevice = vulkan::PhysicalDevice::pickDevice(surface, instance);
	device = std::make_shared<vulkan::Device>(physicalDevice);
	swapchain = std::make_shared<vulkan::Swapchain>(surface, device, window);
	renderPass = std::make_shared<vulkan::RenderPass>(device, swapchain);
	pipeline = std::make_shared<vulkan::Pipeline>(device, swapchain, renderPass);
	for (auto imageView : swapchain->imageViews()) {
		swapChainFramebuffers.push_back(std::make_shared<vulkan::Framebuffer>(imageView, swapchain, renderPass, device));
	}
	commandPool = std::make_shared<vulkan::CommandPool>(device, physicalDevice);
	commandBuffer = std::make_shared<vulkan::CommandBuffer>(device, commandPool);
	/* create sync objects */
	imageAvailableSemaphore = std::make_shared<vulkan::Semaphore>(device);
	renderFinishedSemaphore = std::make_shared<vulkan::Semaphore>(device);
	inFlightFence = std::make_shared<vulkan::Fence>(device);
}

void KaleidoscopeApplication::mainLoop() {
	while (!glfwWindowShouldClose(window->raw())) {
		glfwPollEvents();
		drawFrame();
	}

	device->waitIdle();
}

void KaleidoscopeApplication::cleanup() {
	glfwTerminate();
}

void KaleidoscopeApplication::recordCommandBuffer(vulkan::SharedCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer->raw(), &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass->raw();
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]->raw();
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = swapchain->extent();

	VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer->raw(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer->raw(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->raw());

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchain->extent().width);
	viewport.height = static_cast<float>(swapchain->extent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer->raw(), 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapchain->extent();
	vkCmdSetScissor(commandBuffer->raw(), 0, 1, &scissor);

	vkCmdDraw(commandBuffer->raw(), 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer->raw());

	if (vkEndCommandBuffer(commandBuffer->raw()) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void KaleidoscopeApplication::drawFrame() {
	vkWaitForFences(device->raw(), 1, &inFlightFence->raw(), VK_TRUE, UINT64_MAX);
	vkResetFences(device->raw(), 1, &inFlightFence->raw());

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device->raw(), swapchain->raw(), UINT64_MAX, imageAvailableSemaphore->raw(), VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(commandBuffer->raw(), 0);

	recordCommandBuffer(commandBuffer, imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphore->raw();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer->raw();

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphore->raw();

	if (vkQueueSubmit(device->graphicsQueue(), 1, &submitInfo, inFlightFence->raw()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphore->raw();

	VkSwapchainKHR swapChains[] = {swapchain->raw()};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(device->presentQueue(), &presentInfo);

}

//callbacks
VKAPI_ATTR VkBool32 VKAPI_CALL KaleidoscopeApplication::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

int main() {
	KaleidoscopeApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << "runtime exception: " << std::endl;
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
