#include "RenderPass.h"
#include "Device.h"
#include "Error.h"
#include "Swapchain.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <iostream>
#include <ostream>

namespace vulkan {
	void RenderPassDeleter::operator()(RenderPassData *data) const {
		vkDestroyRenderPass(data->device_->raw(), data->renderPass_, nullptr);
		delete data;
	}

	RenderPass::RenderPass(SharedDevice device, SharedSwapchain swapchain):
		base_type(new RenderPassData{nullptr, device})
	{
		auto createInfo = VkRenderPassCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

		auto colorAttachment = VkAttachmentDescription{};
		colorAttachment.format = swapchain->imageFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		auto colorAttachmentRef = VkAttachmentReference{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &colorAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;

		auto dependency = VkSubpassDependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		auto result = vkCreateRenderPass(device->raw(), &createInfo, nullptr, &raw());
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	VkRenderPass& RenderPass::raw() {
		return get()->renderPass_;
	}
}
