#include "PhysicalDevice.h"
#include "Defs.h"
#include "Instance.h"
#include "Surface.h"
#include "vulkan/vulkan_core.h"
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan {
	PhysicalDevice::PhysicalDevice() {};

	PhysicalDevice::PhysicalDevice(VkPhysicalDevice physicalDevice, SharedSurface surface): physicalDevice_(physicalDevice) {
		//query queues
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());

		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				graphicsQueueFamily_ = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface->raw(), &presentSupport);

			if (presentSupport) {
				presentQueueFamily_ = i;
			}

			if (hasQueueFamilies()) {
				break;
			}

			i++;
		}

		if (!hasQueueFamilies()) {
			return;
		}

		//query surface info
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR	(physicalDevice_, surface->raw(), &surfaceCapabilities_);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface->raw(), &formatCount, nullptr);

		if (formatCount != 0) {
			surfaceFormats_.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface->raw(), &formatCount, surfaceFormats_.data());
		}

		//query present mode info
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface->raw(), &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			presentModes_.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface->raw(), &presentModeCount, presentModes_.data());
		}
	}

	PhysicalDevice PhysicalDevice::pickDevice(SharedSurface surface, SharedInstance instance) {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance->raw(), &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance->raw(), &deviceCount, devices.data());

		for (const auto device : devices) {
			auto tempDevice = PhysicalDevice(device, surface);
			if (tempDevice.isSuitable()) {
				return tempDevice;
			}
		}

		throw std::runtime_error("failed to find a suitable GPU!");
	}


	std::optional<uint32_t> PhysicalDevice::graphicsQueueFamily() {
		return graphicsQueueFamily_;
	}
	std::optional<uint32_t> PhysicalDevice::presentQueueFamily() {
		return presentQueueFamily_;
	}
	std::set<uint32_t> PhysicalDevice::queueFamilies() {
		return std::set<uint32_t>{graphicsQueueFamily_.value(), presentQueueFamily_.value()};
	}
	VkSurfaceCapabilitiesKHR PhysicalDevice::surfaceCapabilities() {
		return surfaceCapabilities_;
	}
	std::vector<VkSurfaceFormatKHR> PhysicalDevice::surfaceFormats() {
		return surfaceFormats_;
	}
	std::vector<VkPresentModeKHR> PhysicalDevice::presentModes() {
		return presentModes_;
	}

	bool PhysicalDevice::isSuitable() {
		//check extensions
		uint32_t extensionsCount;
		vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionsCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extensionsCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		if (!requiredExtensions.empty()) {
			return false;
		}

		//check whether it supports things 
		return graphicsQueueFamily_.has_value() &&
			presentQueueFamily_.has_value() &&
			!surfaceFormats_.empty() &&
			!presentModes_.empty();
	}

	VkExtent2D PhysicalDevice::chooseSwapExtent(SharedWindow window) {
		auto capabilities = surfaceCapabilities();
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(**window, &width, &height);

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

	VkSurfaceFormatKHR PhysicalDevice::chooseSwapSurfaceFormat() {
		for (const auto& availableFormat : surfaceFormats_) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
					availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}
		
		return surfaceFormats_[0];
	}

	VkPresentModeKHR PhysicalDevice::chooseSwapPresentModes() {
		for (const auto& availablePresentMode : presentModes_) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkPhysicalDevice &PhysicalDevice::operator*() {
		return physicalDevice_;
	}

	VkPhysicalDevice &PhysicalDevice::raw() {
		return physicalDevice_;
	}

	bool PhysicalDevice::hasQueueFamilies() {
		return graphicsQueueFamily_.has_value() && presentQueueFamily_.has_value();
	}
}
