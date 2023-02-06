#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <memory>

namespace vulkan {
	class RenderPass;
	using SharedRenderPass = std::shared_ptr<RenderPass>;

	class RenderPass {
		public:
			static SharedRenderPass createShared(VkRenderPassCreateInfo &createInfo, SharedDevice device);
			VkRenderPass& operator*();
			~RenderPass();

		private:
			RenderPass(VkRenderPassCreateInfo &createInfo, SharedDevice device);

			VkRenderPass renderPass_;
			SharedDevice device_;
	};

	class RenderPassFactory {
		public:
			RenderPassFactory(SharedDevice device, SharedSwapchain swapchain);

			RenderPassFactory &defaultConfig();
			SharedRenderPass createShared();

		private:
			VkRenderPassCreateInfo createInfo_{};

			VkAttachmentDescription colorAttachment_{};
			VkAttachmentReference colorAttachmentRef_{};
			VkSubpassDescription subpass_{};
			VkSubpassDependency dependency_{};

			SharedDevice device_;
			SharedSwapchain swapchain_;
	};

}
