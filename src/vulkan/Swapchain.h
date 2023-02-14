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

	struct SwapchainData {
		VkSwapchainKHR swapchain_;
		SharedDevice device_;
		SharedSurface surface_;
		VkFormat imageFormat_;
		VkExtent2D extent_;
		std::vector<VkImage> images_;
		std::vector<SharedImageView> imageViews_;
	};
	struct SwapchainDeleter {
		void operator()(SwapchainData *data) const;
	};

	class Swapchain: public std::unique_ptr<SwapchainData, SwapchainDeleter> {
		public:
			using base_type = std::unique_ptr<SwapchainData, SwapchainDeleter>;

			Swapchain(SharedSurface surface, SharedDevice device, SharedWindow window);
			VkSwapchainKHR& raw();

			SharedDevice device();
			SharedSurface surface();
			VkFormat imageFormat();
			VkExtent2D extent();
			std::vector<VkImage> images();
			std::vector<SharedImageView> imageViews();
	};
}
