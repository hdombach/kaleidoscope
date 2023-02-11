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

	class Pipeline {
		public:
			static SharedPipeline createShared(
					VkGraphicsPipelineCreateInfo &createInfo,
					SharedDevice device,
					SharedSwapchain swapchain,
					SharedRenderPass renderPass);
			SharedPipelineLayout layout();
			VkPipeline& operator*();
			VkPipeline& raw();
			~Pipeline();

		private:
			Pipeline(
					VkGraphicsPipelineCreateInfo &createInfo,
					SharedDevice device,
					SharedSwapchain swapchain,
					SharedRenderPass renderPass);

			VkPipeline pipeline_;

			SharedDevice device_;
			SharedSwapchain swapchain_;
			SharedRenderPass renderPass_;
			SharedPipelineLayout layout_;
	};

	class PipelineFactory {
		public:
			PipelineFactory(
					SharedDevice device,
					SharedSwapchain swapchain,
					SharedRenderPass renderPass);

			PipelineFactory &defaultConfig();
			SharedPipeline createShared();

		private:
			SharedDevice device_;
			SharedSwapchain swapchain_;
			SharedRenderPass renderPass_;


			VkGraphicsPipelineCreateInfo createInfo_{};

			SharedPipelineLayout layout_;
			ShaderModule vertShaderModule_;
			ShaderModule fragShaderModule_;

			VkPipelineShaderStageCreateInfo shaderStages_[2];
			VkPipelineVertexInputStateCreateInfo vertexInputInfo_{};
			VkPipelineInputAssemblyStateCreateInfo inputAssembly_{};
			VkViewport viewport_{};
			VkRect2D scissor_{};
			VkPipelineViewportStateCreateInfo viewportState_{};
			VkPipelineRasterizationStateCreateInfo rasterizer_{};
			VkPipelineMultisampleStateCreateInfo multisampling_{};
			VkPipelineColorBlendAttachmentState colorBlendAttachment_{};
			VkPipelineColorBlendStateCreateInfo colorBlending_{};
			std::vector<VkDynamicState> dynamicStates_;
			VkPipelineDynamicStateCreateInfo dynamicState_{};
	};
}
