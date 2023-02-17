#include "CommandPool.h"
#include "Device.h"
#include "Error.h"
#include "PhysicalDevice.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	void CommandPoolDeleter::operator()(CommandPoolData *data) const {
		vkDestroyCommandPool(data->device_->raw(), data->commandPool_, nullptr);
		delete data;
	}

	CommandPool::CommandPool(SharedDevice device, PhysicalDevice physicalDevice):
		base_type(new CommandPoolData{nullptr})
	{
		auto createInfo = VkCommandPoolCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = physicalDevice.graphicsQueueFamily().value();

		{
			auto data = get();
			auto result = vkCreateCommandPool(device->raw(), &createInfo, nullptr, &data->commandPool_);
			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}
			data->device_ = device;
		}
	}
	
	VkCommandPool &CommandPool::raw() {
		return get()->commandPool_;
	}
}
