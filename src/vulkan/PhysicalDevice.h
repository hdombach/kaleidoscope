#pragma once

#include "Instance.h"
#include "Surface.h"
#include "Window.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>
#include <optional>
#include <set>
namespace vulkan {
	class PhysicalDevice {
		public:
			PhysicalDevice();
			PhysicalDevice(VkPhysicalDevice physicalDevice, SharedSurface surface);

			static PhysicalDevice pickDevice(SharedSurface surface, SharedInstance instance);

			std::optional<uint32_t> graphicsQueueFamily();
			std::optional<uint32_t> presentQueueFamily();
			std::set<uint32_t> queueFamilies();
			VkSurfaceCapabilitiesKHR surfaceCapabilities();
			std::vector<VkSurfaceFormatKHR> surfaceFormats();
			std::vector<VkPresentModeKHR> presentModes();
			bool isSuitable();
			VkExtent2D chooseSwapExtent(SharedWindow window);

			VkSurfaceFormatKHR chooseSwapSurfaceFormat();
			VkPresentModeKHR chooseSwapPresentModes();

			VkPhysicalDevice &operator*();
			VkPhysicalDevice& raw();

		private:
			bool hasQueueFamilies();

			VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
			std::optional<uint32_t> graphicsQueueFamily_;
			std::optional<uint32_t> presentQueueFamily_;
			VkSurfaceCapabilitiesKHR surfaceCapabilities_;
			std::vector<VkSurfaceFormatKHR> surfaceFormats_;
			std::vector<VkPresentModeKHR> presentModes_;
	};
}
