#pragma once

#include "PhysicalDevice.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <set>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Device;
	using SharedDevice = std::shared_ptr<Device>;
	using UniqueDevice = std::unique_ptr<Device>;

	class Device {
		public:
			static SharedDevice createShared(VkDeviceCreateInfo &createInfo, PhysicalDevice physicalDevice);
			static UniqueDevice createUnique(VkDeviceCreateInfo &createInfo, PhysicalDevice physicalDevice);
			void waitIdle();
			VkQueue graphicsQueue();
			VkQueue presentQueue();
			PhysicalDevice physicalDevice();
			VkDevice& operator*();
			~Device();

		private:
			Device(VkDeviceCreateInfo &createInfo, PhysicalDevice physicalDevice);

			VkDevice device_;
			VkQueue graphicsQueue_;
			VkQueue presentQueue_;
			PhysicalDevice physicalDevice_;
	};

	class DeviceFactory {
		public:
			DeviceFactory(PhysicalDevice physicalDevice);

			DeviceFactory &defaultConfig();
			SharedDevice createShared();
			UniqueDevice createUnique();

		private:
			PhysicalDevice physicalDevice_;
			VkDeviceCreateInfo createInfo_{};
			VkPhysicalDeviceFeatures deviceFeatures_{};

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos_;
			std::vector<const char*> extensions_;
			float queuePriority_;
	};
}
