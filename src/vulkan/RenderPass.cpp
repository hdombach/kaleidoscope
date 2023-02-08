#include "RenderPass.h"
#include "Device.h"
#include "Error.h"
#include "Swapchain.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <iostream>
#include <ostream>

namespace vulkan {
	SharedRenderPass RenderPass::createShared(
			VkRenderPassCreateInfo &createInfo,
			SharedDevice device)
	{
		return SharedRenderPass(new RenderPass(createInfo, device));
	}

	VkRenderPass& RenderPass::operator*() {
		return renderPass_;
	}

	VkRenderPass& RenderPass::raw() {
		return renderPass_;
	}

	RenderPass::~RenderPass() {
		vkDestroyRenderPass(**device_, renderPass_, nullptr);
	}

	RenderPass::RenderPass(
			VkRenderPassCreateInfo &createInfo,
			SharedDevice device): device_(device)
	{
		auto result = vkCreateRenderPass(**device_, &createInfo, nullptr, &renderPass_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	/**** factory ****/
	RenderPassFactory::RenderPassFactory(
			SharedDevice device,
			SharedSwapchain swapchain): device_(device), swapchain_(swapchain)
	{
		createInfo_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	}

	RenderPassFactory &RenderPassFactory::defaultConfig() {
		colorAttachment_.format = swapchain_->imageFormat();
		colorAttachment_.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment_.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment_.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment_.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment_.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment_.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment_.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		colorAttachmentRef_.attachment = 0;
		colorAttachmentRef_.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		subpass_.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_.colorAttachmentCount = 1;
		subpass_.pColorAttachments = &colorAttachmentRef_;

		createInfo_.attachmentCount = 1;
		createInfo_.pAttachments = &colorAttachment_;
		createInfo_.subpassCount = 1;
		createInfo_.pSubpasses = &subpass_;

		dependency_.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency_.dstSubpass = 0;
		dependency_.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency_.srcAccessMask = 0;
		dependency_.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency_.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		createInfo_.dependencyCount = 1;
		createInfo_.pDependencies = &dependency_;

		return *this;
	}

	SharedRenderPass RenderPassFactory::createShared() {
		return RenderPass::createShared(createInfo_, device_);
	}
}
