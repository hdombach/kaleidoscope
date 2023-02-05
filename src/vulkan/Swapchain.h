#pragma once

#include "Device.h"
#include "ImageView.h"
#include "PhysicalDevice.h"
#include "Surface.h"
#include "Window.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Swapchain;
	using SharedSwapchain = std::shared_ptr<Swapchain>;
	using UniqueSwapchain = std::unique_ptr<Swapchain>;

	class Swapchain {
		public:
			static SharedSwapchain createShared(
					VkSwapchainCreateInfoKHR &createInfo,
					SharedDevice device,
					SharedSurface surface,
					SharedWindow window);
			static UniqueSwapchain createUnique(
					VkSwapchainCreateInfoKHR &createInfo,
					SharedDevice device,
					SharedSurface surface,
					SharedWindow window);
			VkSwapchainKHR& operator*();
			~Swapchain();

			SharedDevice device();
			SharedSurface surface();
			VkFormat imageFormat();
			VkExtent2D extent();
			std::vector<VkImage> images();
			std::vector<SharedImageView> imageViews();

		private:
			Swapchain(
					VkSwapchainCreateInfoKHR &createInfo,
					SharedDevice device,
					SharedSurface surface,
					SharedWindow window);

			VkSwapchainKHR swapchain_;
			SharedDevice device_;
			SharedSurface surface_;
			VkFormat imageFormat_;
			VkExtent2D extent_;
			std::vector<VkImage> images_;
			std::vector<SharedImageView> imageViews_;
	};

	class SwapchainFactory {
		public:
			SwapchainFactory(
					SharedSurface surface,
					SharedDevice device,
					SharedWindow window);

			SwapchainFactory &defaultConfig();
			SharedSwapchain createShared();
			UniqueSwapchain createUnique();

		private:
			VkSwapchainCreateInfoKHR createInfo_{};

			SharedDevice device_;
			SharedSurface surface_;
			SharedWindow window_;
			std::vector<uint32_t> queueFamilyIndices_;
	};
}
