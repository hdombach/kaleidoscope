#include "Semaphore.h"
#include "Device.h"
#include "Error.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	void SemaphoreDeleter::operator()(SemaphoreData *data) const {
		vkDestroySemaphore(data->device_->raw(), data->semaphore_, nullptr);
		delete data;
	}

	Semaphore::Semaphore(SharedDevice device):
		base_type(new SemaphoreData{nullptr})
	{
		auto createInfo = VkSemaphoreCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		{
			auto data = get();
			VkResult result = vkCreateSemaphore(device->raw(), &createInfo, nullptr, &data->semaphore_);

			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}

			data->device_ = device;
		}
	}

	VkSemaphore &Semaphore::raw() {
		return get()->semaphore_;
	}
}
