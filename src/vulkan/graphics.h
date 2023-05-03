#pragma once

#include "vertex.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <optional>
#include "mainRenderPipeline.h"

namespace vulkan {
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails_ {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class Graphics {
		public:
			Graphics() = default;
			Graphics(const char *name);

			//Graphics(const Graphics& old) = delete;
			//Graphics(Graphics&& old);
			//Graphics& operator=(const Graphics& old) = delete;
			//Graphics& operator=(Graphics&& old);

			//~Graphics();
			void destroy();

			void tick();
			void waitIdle() const;
			GLFWwindow * window();

			VkSurfaceKHR const &surface() const;
			VkPhysicalDevice const &physicalDevice() const;
			VkDevice const &device() const;
			VkSampler mainTextureSampler() const;
			GLFWwindow* window() const;
			VkCommandPool commandPool() const;
			VkQueue graphicsQueue() const;
			VkQueue presentQueue() const;
			VkImageView computeImageView() const;
			MainRenderPipeline &mainRenderPipeline() const;

			VkFormat findSupportedFormat(
					const std::vector<VkFormat>& candidates,
					VkImageTiling tiling,
					VkFormatFeatureFlags features) const;
			VkShaderModule createShaderModule(std::string const &code) const;
			QueueFamilyIndices findQueueFamilies() const;
			void createBuffer(
					VkDeviceSize size,
					VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkBuffer& buffer,
					VkDeviceMemory& bufferMemory) const;
			void createImage(
					uint32_t width,
					uint32_t height,
					uint32_t mipLevels,
					VkFormat format,
					VkImageTiling tiling,
					VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkImage &image,
					VkDeviceMemory &imageMemory) const;
			void transitionImageLayout(
					VkImage image,
					VkFormat format,
					VkImageLayout oldLayout,
					VkImageLayout newLayout,
					uint32_t mipLevels) const;
			void copyBufferToImage(
					VkBuffer buffer,
					VkImage image,
					uint32_t width,
					uint32_t height) const;
			void generateMipmaps(
					VkImage image,
					VkFormat imageFormat,
					int32_t texWidth,
					int32_t texHeight,
					uint32_t mipLevels) const;
			VkImageView createImageView(
					VkImage image,
					VkFormat format,
					VkImageAspectFlags aspectFlags,
					uint32_t mipLevels) const;
			void copyBuffer(
					VkBuffer srcBuffer,
					VkBuffer dstBuffer,
					VkDeviceSize size) const;


		private:
			void initWindow_();
			void initVulkan_();
			void createInstance_();
			void setupDebugMessenger_();
			void pickPhysicalDevice_();
			bool isDeviceSuitable_(VkPhysicalDevice device);
			bool checkDeviceExtensionSupport_(VkPhysicalDevice device);
			void createLogicalDevice_();
			void createSwapChain_();
			void createComputeDescriptorSetLayout_();
			void createComputePipeline_();
			void createCommandPool_();
			void loadModel_();
			void createDescriptorPool_();
			void createComputeDescriptorSets_();
			void createComputeCommandBuffers_();
			void createSyncObjects_();
			void createSurface_();
			void recordCommandBuffer_(VkCommandBuffer commandBuffer, uint32_t imageIndex);
			void recordComputeCommandBuffer_(VkCommandBuffer commandBuffer);
			void createTextureSampler_();
			void createComputeResultTexture_();

			void initImgui_();

			void drawFrame_();
			void drawUi_();
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
			QueueFamilyIndices findQueueFamilies_(VkPhysicalDevice device) const;
			SwapChainSupportDetails_ querySwapChainSupport_(VkPhysicalDevice device);
			VkSurfaceFormatKHR chooseSwapSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR chooseSwapPresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes);
			VkExtent2D chooseSwapExtent_(const VkSurfaceCapabilitiesKHR& capabilities);
			VkShaderModule createShaderModule_(const std::string& code) const;
			uint32_t findMemoryType_(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
			void createBuffer_(
					VkDeviceSize size,
					VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkBuffer& buffer,
					VkDeviceMemory& bufferMemory) const;
			void copyBuffer_(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
			void createImage_(
					uint32_t width,
					uint32_t height,
					uint32_t mipLevels,
					VkFormat format,
					VkImageTiling tiling,
					VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkImage &image,
					VkDeviceMemory &imageMemory) const;
			VkCommandBuffer beginSingleTimeCommands_() const;
			void endSingleTimeCommands_(VkCommandBuffer commandBuffer) const;
			void transitionImageLayout_(
					VkImage image,
					VkFormat format,
					VkImageLayout oldLayout,
					VkImageLayout newLayout,
					uint32_t mipLevels) const;
			void copyBufferToImage_(
					VkBuffer buffer,
					VkImage image,
					uint32_t width,
					uint32_t height) const;
			VkImageView createImageView_(
					VkImage image,
					VkFormat format,
					VkImageAspectFlags aspectFlags,
					uint32_t mipLevels) const;
			VkFormat findSupportedFormat_(
					const std::vector<VkFormat>& candidates,
					VkImageTiling tiling,
					VkFormatFeatureFlags features) const;
			VkFormat findDepthFormat_();
			bool hasStencilComponent_(VkFormat format) const;
			void generateMipmaps_(
					VkImage image,
					VkFormat imageFormat,
					int32_t texWidth,
					int32_t texHeight,
					uint32_t mipLevels) const;

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
			VkQueue computeQueue_;
			VkSurfaceKHR surface_;
			VkSwapchainKHR swapChain_;
			std::vector<VkImage> swapChainImages_;
			VkFormat swapChainImageFormat_;
			VkExtent2D swapChainExtent_;
			VkRenderPass renderPass_;
			VkDescriptorSetLayout descriptorSetLayout_;
			VkDescriptorSetLayout computeDescriptorSetLayout_;
			VkDescriptorPool descriptorPool_;
			std::vector<VkDescriptorSet> descriptorSets_;
			std::vector<VkDescriptorSet> computeDescriptorSets_;
			VkPipelineLayout pipelineLayout_;
			VkPipelineLayout computePipelineLayout_;
			VkPipeline graphicsPipeline_;
			VkPipeline computePipeline_;
			std::vector<VkFramebuffer> swapChainFramebuffers_;
			VkCommandPool commandPool_;
			std::vector<Vertex> vertices_;
			std::vector<uint32_t> indices_;
			VkBuffer vertexBuffer_;
			VkDeviceMemory vertexBufferMemory_;
			VkBuffer indexBuffer_;
			VkDeviceMemory indexBufferMemory_;
			uint32_t mipLevels_;
			VkImage textureImage_;
			VkDeviceMemory textureImageMemory_;
			VkImageView textureImageView_;
			VkSampler textureSampler_;
			VkImage depthImage_;
			VkDeviceMemory depthImageMemory_;
			VkImageView depthImageView_;
			VkDeviceMemory computeResultMemory_;
			VkImage computeResultImage_;
			VkImageView computeResultImageView_;
			std::vector<VkBuffer> uniformBuffers_;
			std::vector<VkDeviceMemory> uniformBuffersMemory_;
			std::vector<void*> uniformBuffersMapped_;
			std::vector<VkCommandBuffer> commandBuffers_;
			std::vector<VkCommandBuffer> computeCommandBuffers_;
			std::vector<VkSemaphore> imageAvailableSemaphores_;
			std::vector<VkSemaphore> renderFinishedSemaphores_;
			std::vector<VkSemaphore> computeFinishedSemaphores_;
			std::vector<VkFence> inFlightFences_;
			std::vector<VkFence> computeInFlightFences_;
			std::vector<VkImageView> swapChainImageViews_;

			std::unique_ptr<MainRenderPipeline> mainRenderPipeline_;

			//imgui stuff
			VkDescriptorPool imguiPool_;

			bool framebufferResized_ = false;
			uint32_t currentFrame_ = 0;
	};
}
