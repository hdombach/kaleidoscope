#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Device.h"
#include "Error.h"
#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

namespace vulkan {
	CommandBuffer::CommandBuffer(SharedDevice device, SharedCommandPool commandPool):
		base_type(new CommandBufferData{nullptr})
	{
		auto createInfo = VkCommandBufferAllocateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		createInfo.commandPool = commandPool->raw();
		createInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		createInfo.commandBufferCount = 1;

		{
			auto data = get();
			auto result = vkAllocateCommandBuffers(device->raw(), &createInfo, &data->commnadBuffer_);
			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}
			data->device_ = device;
		}
	}

	VkCommandBuffer &CommandBuffer::raw() {
		return get()->commnadBuffer_;
	}
}
