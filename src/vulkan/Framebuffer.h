#pragma once

#include "Device.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Framebuffer;
	using SharedFramebuffer = std::shared_ptr<Framebuffer>;

	struct FramebufferData {
		VkFramebuffer framebuffer_;
		SharedDevice device_;
	};
	struct FramebufferDeleter {
		void operator()(FramebufferData *data) const;
	};

	class Framebuffer: public std::unique_ptr<FramebufferData, FramebufferDeleter> {
		public:
			using base_type = std::unique_ptr<FramebufferData, FramebufferDeleter>;
			Framebuffer(SharedImageView, SharedSwapchain, SharedRenderPass, SharedDevice);
			VkFramebuffer& raw();
	};
}
