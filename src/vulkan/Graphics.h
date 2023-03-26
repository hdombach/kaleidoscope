#pragma once

#include "vertex.h"
#include "vulkan/vulkan_core.h"
#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <optional>

namespace vulkan {
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Graphics {
		public:
			Graphics() = default;
			Graphics(const char *name);
			void tick();
			void waitIdle();
			GLFWwindow * window();

		private:
			void initWindow_();
			void initVulkan_();
			void recreateSwapChain_();
			void createInstance_();
			void setupDebugMessenger_();
			void pickPhysicalDevice_();
			bool isDeviceSuitable_(VkPhysicalDevice device);
			bool checkDeviceExtensionSupport_(VkPhysicalDevice device);
			void createLogicalDevice_();
			void createSwapChain_();
			void createImageViews_();
			void createRenderPass_();
			void createDescriptorSetLayout_();
			void createGraphicsPipeline_();
			void createFramebuffers_();
			void createCommandPool_();
			void createTextureImage_();
			void createVertexBuffer_();
			void createIndexBuffer_();
			void createUniformBuffers_();
			void createDescriptorPool_();
			void createDescriptorSets_();
			void createCommandBuffers_();
			void createSyncObjects_();
			void createSurface_();
			void recordCommandBuffer_(VkCommandBuffer commandBuffer, uint32_t imageIndex);
			void createTextureImageView_();
			void createTextureSampler_();
			void createDepthResources_();

			void drawFrame_();
			void updateUniformBuffer_(uint32_t currentImage);
			bool checkValidationLayerSupport_();
			void cleanupSwapChain_();
			void cleanup_();

			VkResult createDebugUtilsMessengerEXT(
					VkInstance instance,
					const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
					const VkAllocationCallbacks* pAllocator,
					VkDebugUtilsMessengerEXT* pDebugMessenger);
			void destroyDebugUtilsMessengerEXT_(
					VkInstance instance,
					VkDebugUtilsMessengerEXT debugMessenger,
					const VkAllocationCallbacks* pAllocator);
			static void framebufferResizeCallback_(GLFWwindow* window, int width, int height);
			std::vector<const char*> getRequiredExtensions_();
			void populateDebugMessengerCreateInfo_(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
			QueueFamilyIndices findQueueFamilies_(VkPhysicalDevice device);
			SwapChainSupportDetails querySwapChainSupport_(VkPhysicalDevice device);
			VkSurfaceFormatKHR chooseSwapSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR chooseSwapPresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes);
			VkExtent2D chooseSwapExtent_(const VkSurfaceCapabilitiesKHR& capabilities);
			VkShaderModule createShaderModule_(const std::string& code);
			uint32_t findMemoryType_(uint32_t typeFilter, VkMemoryPropertyFlags properties);
			void createBuffer_(
					VkDeviceSize size,
					VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkBuffer& buffer,
					VkDeviceMemory& bufferMemory);
			void copyBuffer_(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
			void createImage_(
					uint32_t width,
					uint32_t height,
					VkFormat format,
					VkImageTiling tiling,
					VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkImage &image,
					VkDeviceMemory &imageMemory);
			VkCommandBuffer beginSingleTimeCommands_();
			void endSingleTimeCommands_(VkCommandBuffer commandBuffer);
			void transitionImageLayout_(
					VkImage image,
					VkFormat format,
					VkImageLayout oldLayout,
					VkImageLayout newLayout);
			void copyBufferToImage_(
					VkBuffer buffer,
					VkImage image,
					uint32_t width,
					uint32_t height);
			VkImageView createImageView_(
					VkImage image,
					VkFormat format,
					VkImageAspectFlags aspectFlags);
			VkFormat findSupportedFormat_(
					const std::vector<VkFormat>& candidates,
					VkImageTiling tiling,
					VkFormatFeatureFlags features);
			VkFormat findDepthFormat_();
			bool hasStencilComponent_(VkFormat format);

			static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageTypeata,
				const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
				void* pUserData);


			const char *name_;
			GLFWwindow* window_;
			VkInstance instance_;
			VkDebugUtilsMessengerEXT debugMessenger_;
			VkPhysicalDevice physicalDevice_;
			VkDevice device_;
			VkQueue graphicsQueue_;
			VkQueue presentQueue_;
			VkSurfaceKHR surface_;
			VkSwapchainKHR swapChain_;
			std::vector<VkImage> swapChainImages_;
			VkFormat swapChainImageFormat_;
			VkExtent2D swapChainExtent_;
			VkRenderPass renderPass_;
			VkDescriptorSetLayout descriptorSetLayout_;
			VkDescriptorPool descriptorPool_;
			std::vector<VkDescriptorSet> descriptorSets_;
			VkPipelineLayout pipelineLayout_;
			VkPipeline graphicsPipeline_;
			std::vector<VkFramebuffer> swapChainFramebuffers_;
			VkCommandPool commandPool_;
			VkBuffer vertexBuffer_;
			VkDeviceMemory vertexBufferMemory_;
			VkBuffer indexBuffer_;
			VkDeviceMemory indexBufferMemory_;
			VkImage textureImage_;
			VkDeviceMemory textureImageMemory_;
			VkImageView textureImageView_;
			VkSampler textureSampler_;
			VkImage depthImage_;
			VkDeviceMemory depthImageMemory_;
			VkImageView depthImageView_;
			std::vector<VkBuffer> uniformBuffers_;
			std::vector<VkDeviceMemory> uniformBuffersMemory_;
			std::vector<void*> uniformBuffersMapped_;
			std::vector<VkCommandBuffer> commandBuffers_;
			std::vector<VkSemaphore> imageAvailableSemaphores_;
			std::vector<VkSemaphore> renderFinishedSemaphores_;
			std::vector<VkFence> inFlightFences_;
			std::vector<VkImageView> swapChainImageViews_;

			bool framebufferResized_ = false;
			uint32_t currentFrame_ = 0;
			std::vector<Vertex> vertices_ = {
				{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
				{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
				{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
				{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

				{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
				{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
				{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
				{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
			};
			std::vector<uint16_t> indices_ = {
				0, 1, 2, 2, 3, 0,
				4, 5, 6, 6, 7, 4,
			};
	};
}
