#include "Device.h"
#include "Defs.h"
#include "Error.h"
#include "PhysicalDevice.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	void DeviceDeleter::operator()(DeviceData *data) const {
		vkDestroyDevice(data->device_, nullptr);
		delete data;
	}

	Device::Device(PhysicalDevice physicalDevice):
		base_type(new DeviceData{nullptr, nullptr, nullptr, physicalDevice})
	{
		auto createInfo = VkDeviceCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		auto uniqueQueueFamilies = physicalDevice.queueFamilies();
		auto queueCreateInfos = std::vector<VkDeviceQueueCreateInfo>();
		float queuePriority = 1.0f;

		for (uint32_t queueFamily : uniqueQueueFamilies) {
			auto queueCreateInfo = VkDeviceQueueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		auto deviceFeatures = VkPhysicalDeviceFeatures{};
		createInfo.pEnabledFeatures = &deviceFeatures;

		auto extensions = std::vector<const char*>();
		extensions.push_back("VK_KHR_portability_subset");
		extensions.insert(extensions.end(), DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (ENABLE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		auto result = vkCreateDevice(physicalDevice.raw(), &createInfo, nullptr, &raw());
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		vkGetDeviceQueue(raw(), physicalDevice.graphicsQueueFamily().value(), 0, &get()->graphicsQueue_);
		vkGetDeviceQueue(raw(), physicalDevice.presentQueueFamily().value(), 0, &get()->presentQueue_);
	}

	void Device::waitIdle() {
		vkDeviceWaitIdle(raw());
	}

	VkQueue Device::graphicsQueue() {
		return get()->graphicsQueue_;
	}

	VkQueue Device::presentQueue() {
		return get()->presentQueue_;
	}

	PhysicalDevice Device::physicalDevice() {
		return get()->physicalDevice_;
	}

	VkDevice& Device::raw() {
		return get()->device_;
	}
}
