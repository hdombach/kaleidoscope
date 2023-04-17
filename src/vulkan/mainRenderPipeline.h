#pragma once

#include <vulkan/vulkan.h>
#include "vulkan/vulkan_core.h"
#include <vector>

namespace vulkan {
	struct SwapchainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Graphics;

	class MainRenderPipeline {
		public:
			MainRenderPipeline(Graphics const &graphics);
			~MainRenderPipeline();
			static SwapchainSupportDetails querySwapchainSupport_(VkPhysicalDevice device, Graphics const &graphics);

		private:
			void createSwapchain_();
			void createRenderPass_();
			void createDescriptorSetLayout_();
			void createDescriptorPool_();
			void createUniformBuffers_();
			void createTextureImage_();
			void createTextureImageView_();
			void createDescriptorSets_();
			void createPipeline_();

			void recreateSwapchain_();

			VkSurfaceFormatKHR chooseSwapchainSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR chooseSwapchainPresentMode_(const std::vector<VkPresentModeKHR>& availabePresentModes);
			VkExtent2D chooseSwapchainExtent_(const VkSurfaceCapabilitiesKHR& capabilities);
			VkFormat findDepthFormat_();

			Graphics const &graphics_;
			VkPipelineLayout pipelineLayout_;
			VkPipeline pipeline_;
			VkRenderPass renderPass_;
			VkSwapchainKHR swapchain_;
			std::vector<VkImage> swapchainImages_;
			VkFormat swapchainImageFormat_;
			VkExtent2D swapchainExtent_;
			std::vector<VkFramebuffer> swapchainFramebuffers_;
			VkDescriptorSetLayout descriptorSetLayout_;
			std::vector<VkDescriptorSet> descriptorSets_;
			VkDescriptorPool descriptorPool_;
			std::vector<VkBuffer> uniformBuffers_;
			std::vector<VkDeviceMemory> uniformBuffersMemory_;
			std::vector<void*> uniformBuffersMapped_;

			VkImage textureImage_;
			VkDeviceMemory textureImageMemory_;
			VkImageView textureImageView_;

			uint32_t mipLevels_;
	};
}
