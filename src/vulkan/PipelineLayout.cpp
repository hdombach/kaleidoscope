#include "PipelineLayout.h"
#include "Device.h"
#include "Error.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	void PipelineLayoutDeleter::operator()(PipelineLayoutData *data) const {
		vkDestroyPipelineLayout(data->device_->raw(), data->pipelineLayout_, nullptr);
		delete data;
	}

	PipelineLayout::PipelineLayout(SharedDevice device, SharedSwapchain swapchain):
		base_type(new PipelineLayoutData{nullptr})
	{
		auto createInfo = VkPipelineLayoutCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		createInfo.setLayoutCount = 0;
		createInfo.pSetLayouts = nullptr;
		createInfo.pushConstantRangeCount = 0;
		createInfo.pPushConstantRanges = nullptr;

		{
			auto data = get();
			auto result = vkCreatePipelineLayout(device->raw(), &createInfo, nullptr, &data->pipelineLayout_);
			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}
			data->swapchain_ = swapchain;
			data->device_ = device;
		}
	}

	VkPipelineLayout& PipelineLayout::raw() {
		return get()->pipelineLayout_;
	}
}
