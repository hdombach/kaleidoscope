#pragma once

#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DebugUtilsMessenger.h"
#include "Device.h"
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
#include <memory>
#include <vector>
namespace vulkan {
	class Graphics {
		public:
			Graphics() = default;
			Graphics(SharedWindow window);

			void drawFrame();
			void waitIdle();

			static const size_t MAX_FRAMES_IN_FLIGHT = 2;

		private:
			void recordCommandBuffer(SharedCommandBuffer commandBuffer, uint32_t imageIndex);

			SharedWindow window_;
			SharedInstance instance_;
			SharedDebugUtilsMessenger debugMessenger_;
			PhysicalDevice physicalDevice_;
			SharedDevice device_;
			SharedSurface surface_;
			SharedSwapchain swapchain_;
			SharedRenderPass renderPass_;
			SharedPipeline pipeline_;
			std::vector<SharedFramebuffer> swapChainFramebuffers_;
			SharedCommandPool commandPool_;
			std::vector<SharedCommandBuffer> commandBuffers_;
			std::vector<SharedSemaphore> imageAvailableSemaphores_;
			std::vector<SharedSemaphore> renderFinishedSemaphores_;
			std::vector<SharedFence> inFlightFences_;

			uint32_t currentFrame_ = 0;
	};
}
