#include "PhysicalDevice.h"
#include "Defs.h"
#include "Instance.h"
#include "Surface.h"
#include "vulkan/vulkan_core.h"
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
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, **surface, &presentSupport);

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
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR	(physicalDevice_, **surface, &surfaceCapabilities_);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, **surface, &formatCount, nullptr);

		if (formatCount != 0) {
			surfaceFormats_.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, **surface, &formatCount, surfaceFormats_.data());
		}

		//query present mode info
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, **surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			presentModes_.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, **surface, &presentModeCount, presentModes_.data());
		}
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

	VkPhysicalDevice &PhysicalDevice::operator*() {
		return physicalDevice_;
	}

	bool PhysicalDevice::hasQueueFamilies() {
		return graphicsQueueFamily_.has_value() && presentQueueFamily_.has_value();
	}

	PhysicalDeviceFactory::PhysicalDeviceFactory(SharedSurface surface, SharedInstance instance):
		surface_(surface),
		instance_(instance) {}

	PhysicalDevice PhysicalDeviceFactory::pickDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(**instance_, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(**instance_, &deviceCount, devices.data());

		for (const auto& device : devices) {
			auto tempDevice = PhysicalDevice(device, surface_);
			if (tempDevice.isSuitable()) {
				return tempDevice;
			}
		}

		throw std::runtime_error("failed to find a suitable GPU!");
	}
}
