#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <memory>

namespace vulkan {
	class RenderPass;
	using SharedRenderPass = std::shared_ptr<RenderPass>;

	struct RenderPassData {
		VkRenderPass renderPass_;
		SharedDevice device_;
	};
	struct RenderPassDeleter {
		void operator()(RenderPassData *data) const;
	};

	class RenderPass: public std::unique_ptr<RenderPassData, RenderPassDeleter> {
		public:

			using base_type = std::unique_ptr<RenderPassData, RenderPassDeleter>;
			RenderPass(SharedDevice device, SharedSwapchain swapchain);
			VkRenderPass& raw();
	};
}
