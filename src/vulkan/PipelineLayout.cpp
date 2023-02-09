#include "PipelineLayout.h"
#include "Device.h"
#include "Error.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	SharedPipelineLayout PipelineLayout::createShared(
			VkPipelineLayoutCreateInfo &createInfo,
			SharedSwapchain swapchain,
			SharedDevice device) 
	{
		return SharedPipelineLayout(new PipelineLayout(createInfo, swapchain, device));
	}

	VkPipelineLayout& PipelineLayout::operator*() {
		return pipelineLayout_;
	}
	
	VkPipelineLayout& PipelineLayout::raw() {
		return pipelineLayout_;
	}

	PipelineLayout::~PipelineLayout() {
		vkDestroyPipelineLayout(device_->raw(), pipelineLayout_, nullptr);
	}

	PipelineLayout::PipelineLayout(
			VkPipelineLayoutCreateInfo &createInfo,
			SharedSwapchain swapchain,
			SharedDevice device):
		device_(device),
		swapchain_(swapchain)
	{
		auto result = vkCreatePipelineLayout(device->raw(), &createInfo, nullptr, &pipelineLayout_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	/**** factory ****/

	PipelineLayoutFactory::PipelineLayoutFactory(
			SharedSwapchain swapchain,
			SharedDevice device):
		swapchain_(swapchain),
		device_(device)
	{
		createInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	}

	PipelineLayoutFactory &PipelineLayoutFactory::defaultConfig() {
		createInfo_.setLayoutCount = 0;
		createInfo_.pSetLayouts = nullptr;
		createInfo_.pushConstantRangeCount = 0;
		createInfo_.pPushConstantRanges = nullptr;

		return *this;
	}

	SharedPipelineLayout PipelineLayoutFactory::createShared() {
		return PipelineLayout::createShared(createInfo_, swapchain_, device_);
	}
}
