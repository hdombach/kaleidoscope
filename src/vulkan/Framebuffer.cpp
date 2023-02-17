#include "Framebuffer.h"
#include "Device.h"
#include "Error.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	void FramebufferDeleter::operator()(FramebufferData *data) const {
		vkDestroyFramebuffer(data->device_->raw(), data->framebuffer_, nullptr);
		delete data;
	}

	Framebuffer::Framebuffer(
			SharedImageView imageView,
			SharedSwapchain swapchain,
			SharedRenderPass renderpass,
			SharedDevice device):
		base_type(new FramebufferData{nullptr})
	{
		VkImageView attachments[] = {
			imageView->raw()
		};

		auto createInfo = VkFramebufferCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderpass->raw();
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = swapchain->extent().width;
		createInfo.height = swapchain->extent().height;
		createInfo.layers = 1;

		{
			auto data = get();
			auto result = vkCreateFramebuffer(device->raw(), &createInfo, nullptr, &data->framebuffer_);
			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}
			data->device_ = device;
		}
	}

	VkFramebuffer& Framebuffer::raw() {
		return get()->framebuffer_;
	}
}
