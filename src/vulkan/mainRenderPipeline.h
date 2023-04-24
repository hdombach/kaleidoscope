#pragma once

#include <vulkan/vulkan.h>
#include "vertex.h"
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
			void submit(uint32_t frameIndex, VkSemaphore previousSemaphore);
			void loadVertices(std::vector<Vertex> vertices, std::vector<uint32_t> indices);

		private:
			void createSyncObjects_();
			void createCommandBuffers_();
			void createSwapchain_();
			void createSwapchainImageViews_();
			void createSwapchainFramebuffers_();
			void createRenderPass_();
			void createDescriptorSetLayout_();
			void createDescriptorPool_();
			void createUniformBuffers_();
			void createTextureImage_();
			void createTextureImageView_();
			void createDepthResources_();
			void createDescriptorSets_();
			void createPipeline_();

			void recordCommandBuffer_(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t frameIndex);
			void updateUniformBuffer_(uint32_t currentImage);

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
			VkBuffer vertexBuffer_;
			VkDeviceMemory vertexBufferMemory_;
			VkBuffer indexBuffer_;
			VkDeviceMemory indexBufferMemory_;
			uint32_t indexCount_;
			std::vector<VkBuffer> uniformBuffers_;
			std::vector<VkDeviceMemory> uniformBuffersMemory_;
			std::vector<void*> uniformBuffersMapped_;
			std::vector<VkFence> inFlightFences_;
			std::vector<VkSemaphore> imageAvailableSemaphores_;
			std::vector<VkSemaphore> renderFinishedSemaphores_;
			std::vector<VkCommandBuffer> commandBuffers_;

			VkImage textureImage_;
			VkDeviceMemory textureImageMemory_;
			VkImageView textureImageView_;
			VkImage depthImage_;
			VkDeviceMemory depthImageMemory_;
			VkImageView depthImageView_;
			std::vector<VkImageView> swapchainImageViews_;

			uint32_t mipLevels_;
	};
}
