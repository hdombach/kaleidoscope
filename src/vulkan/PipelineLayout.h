#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <memory>

namespace vulkan {
	class PipelineLayout;
	using SharedPipelineLayout = std::shared_ptr<PipelineLayout>;

	struct PipelineLayoutData {
		VkPipelineLayout pipelineLayout_;
		SharedSwapchain swapchain_;
		SharedDevice device_;
	};
	struct PipelineLayoutDeleter {
		void operator()(PipelineLayoutData *data) const;
	};

	class PipelineLayout: std::unique_ptr<PipelineLayoutData, PipelineLayoutDeleter> {
		public:
			using base_type = std::unique_ptr<PipelineLayoutData, PipelineLayoutDeleter>;

			PipelineLayout(SharedDevice device, SharedSwapchain swapchain);
			VkPipelineLayout& raw();
	};
}
