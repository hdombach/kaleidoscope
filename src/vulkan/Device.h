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

	struct DeviceData {
		VkDevice device_;

		VkQueue graphicsQueue_;
		VkQueue presentQueue_;
		PhysicalDevice physicalDevice_;
	};
	struct DeviceDeleter {
		void operator()(DeviceData *data) const;
	};

	class Device: public std::unique_ptr<DeviceData, DeviceDeleter> {
		public:
			using base_type = std::unique_ptr<DeviceData, DeviceDeleter>;

			Device() = default;
			Device(PhysicalDevice physicalDevice);

			void waitIdle();
			VkQueue graphicsQueue();
			VkQueue presentQueue();
			PhysicalDevice physicalDevice();
			VkDevice& raw();

	};
}
