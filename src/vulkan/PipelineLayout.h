#pragma once

#include "Device.h"
#include "Swapchain.h"
#include "vulkan/vulkan.h"
#include "vulkan/vulkan_core.h"
#include <memory>

namespace vulkan {
	class PipelineLayout;
	using SharedPipelineLayout = std::shared_ptr<PipelineLayout>;

	class PipelineLayout {
		public:
			static SharedPipelineLayout createShared(
					VkPipelineLayoutCreateInfo &createInfo,
					SharedSwapchain swapchain,
					SharedDevice device);
			VkPipelineLayout& operator*();
			VkPipelineLayout& raw();
			~PipelineLayout();

		private:
			PipelineLayout(
					VkPipelineLayoutCreateInfo &createInfo,
					SharedSwapchain swapchain,
					SharedDevice device);

			VkPipelineLayout pipelineLayout_;
			SharedSwapchain swapchain_;
			SharedDevice device_;
	};

	class PipelineLayoutFactory {
		public:
			PipelineLayoutFactory(SharedSwapchain swapchain, SharedDevice device);

			PipelineLayoutFactory &defaultConfig();
			SharedPipelineLayout createShared();

		private:
			SharedSwapchain swapchain_;
			SharedDevice device_;

			VkPipelineLayoutCreateInfo createInfo_{};
	};
}