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

	class Framebuffer {
		public:
			Framebuffer();

			Framebuffer(SharedImageView, SharedSwapchain, SharedRenderPass, SharedDevice);

			Framebuffer(const Framebuffer &) = delete;
			Framebuffer& operator=(const Framebuffer &) = delete;
			Framebuffer(Framebuffer&&);
			Framebuffer& operator=(Framebuffer &&);
			VkFramebuffer& operator*();
			VkFramebuffer& raw();
			~Framebuffer();

		private:
			VkFramebuffer framebuffer_=nullptr;
			SharedDevice device_;
	};
}
