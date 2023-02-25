#include "Fence.h"
#include "Device.h"
#include "Error.h"
#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

namespace vulkan {
	void FenceDeleter::operator()(FenceData *data) const {
		vkDestroyFence(data->device_->raw(), data->fence_, nullptr);
		delete data;
	}

	Fence::Fence(SharedDevice device):
		base_type(new FenceData{nullptr})
	{
		auto createInfo = VkFenceCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		{
			auto data = get();

			auto result = vkCreateFence(device->raw(), &createInfo, nullptr, &data->fence_);

			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}

			data->device_ = device;
		}
	}

	VkFence& Fence::raw() {
		return get()->fence_;
	}
}
