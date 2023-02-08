#include "Device.h"
#include "Defs.h"
#include "Error.h"
#include "PhysicalDevice.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	SharedDevice Device::createShared(
			VkDeviceCreateInfo &createInfo,
			PhysicalDevice physicalDevice)
	{
		return SharedDevice(new Device(createInfo, physicalDevice));
	}

	UniqueDevice Device::createUnique(
			VkDeviceCreateInfo &createInfo,
			PhysicalDevice physicalDevice)
	{
		return UniqueDevice(new Device(createInfo, physicalDevice));
	}

	void Device::waitIdle() {
		vkDeviceWaitIdle(device_);
	}

	VkQueue Device::graphicsQueue() {
		return graphicsQueue_;
	}

	VkQueue Device::presentQueue() {
		return presentQueue_;
	}

	PhysicalDevice Device::physicalDevice() {
		return physicalDevice_;
	}

	VkDevice& Device::operator*() {
		return device_;
	}

	VkDevice& Device::raw() {
		return device_;
	}

	Device::~Device() {
		vkDestroyDevice(device_, nullptr);
	}

	Device::Device(
			VkDeviceCreateInfo &createInfo,
			PhysicalDevice physicalDevice): physicalDevice_(physicalDevice)
	{
		auto result = vkCreateDevice(*physicalDevice, &createInfo, nullptr, &device_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		vkGetDeviceQueue(
				device_,
				physicalDevice.graphicsQueueFamily().value(),
				0,
				&graphicsQueue_);
		vkGetDeviceQueue(device_,
				physicalDevice.presentQueueFamily().value(),
				0,
				&presentQueue_);
	}

	/**** Factory ****/

	DeviceFactory::DeviceFactory(PhysicalDevice physicalDevice):
		physicalDevice_(physicalDevice)
	{
		createInfo_.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	}

	DeviceFactory &DeviceFactory::defaultConfig() {
		auto uniqueQueueFamilies = physicalDevice_.queueFamilies();

		queueCreateInfos_ = std::vector<VkDeviceQueueCreateInfo>();

		queuePriority_ = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority_;
			queueCreateInfos_.push_back(queueCreateInfo);
		}

		createInfo_.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos_.size());
		createInfo_.pQueueCreateInfos = queueCreateInfos_.data();

		createInfo_.pEnabledFeatures = &deviceFeatures_;

		extensions_ = std::vector<const char*>();
		extensions_.push_back("VK_KHR_portability_subset");
		extensions_.insert(extensions_.end(), DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
		createInfo_.enabledExtensionCount = static_cast<uint32_t>(extensions_.size());
		createInfo_.ppEnabledExtensionNames = extensions_.data();

		if (ENABLE_VALIDATION_LAYERS) {
			createInfo_.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo_.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		} else {
			createInfo_.enabledLayerCount = 0;
		}

		return *this;
	}

	SharedDevice DeviceFactory::createShared() {
		return Device::createShared(createInfo_, physicalDevice_);
	}
	UniqueDevice DeviceFactory::createUnique() {
		return Device::createUnique(createInfo_, physicalDevice_);
	}
}
