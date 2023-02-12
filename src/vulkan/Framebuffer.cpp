#include "Framebuffer.h"
#include "Device.h"
#include "Error.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	Framebuffer::Framebuffer(): framebuffer_(nullptr) {}

	Framebuffer::Framebuffer(
			SharedImageView imageView,
			SharedSwapchain swapchain,
			SharedRenderPass renderpass,
			SharedDevice device):
		device_(device)
	{
		VkImageView attachments[] = {
			imageView->raw()
		};

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderpass->raw();
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = swapchain->extent().width;
		createInfo.height = swapchain->extent().height;
		createInfo.layers = 1;

		auto result = vkCreateFramebuffer(device_->raw(), &createInfo, nullptr, &framebuffer_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	Framebuffer::Framebuffer(Framebuffer&& other) {
		device_ = other.device_;
		framebuffer_ = other.framebuffer_;
		other.framebuffer_ = nullptr;
	}

	Framebuffer& Framebuffer::operator=(Framebuffer&& other) {
		destroy();
		device_ = other.device_;
		framebuffer_ = other.framebuffer_;
		other.framebuffer_ = nullptr;
		return *this;
	}

	VkFramebuffer& Framebuffer::operator*() {
		return framebuffer_;
	}

	VkFramebuffer& Framebuffer::raw() {
		return framebuffer_;
	}

	Framebuffer::~Framebuffer() {
		destroy();
	}

	void Framebuffer::destroy() {
		if (framebuffer_ != nullptr) {
			vkDestroyFramebuffer(device_->raw(), framebuffer_, nullptr);
		}
	}
}
