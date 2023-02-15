#pragma once

#include "Device.h"
#include "PipelineLayout.h"
#include "RenderPass.h"
#include "ShaderModule.h"
#include "Swapchain.h"
#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace vulkan {
	class Pipeline;
	using SharedPipeline = std::shared_ptr<Pipeline>;

	struct PipelineData {
		VkPipeline pipeline_;
		SharedDevice device_;
		SharedSwapchain swapchain_;
		SharedRenderPass renderPass_;
		SharedPipelineLayout layout_;
	};
	struct PipelineDeleter {
		void operator()(PipelineData *data) const;
	};

	class Pipeline: public std::unique_ptr<PipelineData, PipelineDeleter> {
		public:
			using base_type = std::unique_ptr<PipelineData, PipelineDeleter>;

			Pipeline(SharedDevice device, SharedSwapchain swapchain, SharedRenderPass renderPass);
			VkPipeline& raw();

			SharedPipelineLayout layout();
	};
}
