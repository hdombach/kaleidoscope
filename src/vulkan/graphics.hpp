#pragma once

#include <functional>
#include <vector>
#include <optional>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include "semaphore.hpp"
#include "fence.hpp"
#include "imageView.hpp"
#include "../util/errors.hpp"

namespace vulkan {
	class Semaphore;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	class Graphics {
		public:
			struct SwapchainSupportDetails;

			static Graphics *DEFAULT;
			static void initDefault(const char *name);
			static void deleteDefault();

			~Graphics();

			void waitIdle() const;
			GLFWwindow * window();

			VkSurfaceKHR const &surface() const;
			VkPhysicalDevice const &physicalDevice() const;
			VkDevice const &device() const;
			VkInstance const &instance() const;
			VkSampler mainTextureSampler() const;
			GLFWwindow* window() const;
			VkCommandPool commandPool() const;
			VkQueue graphicsQueue() const;
			VkQueue presentQueue() const;
			ImageView const &compute_image_view() const;
			SwapchainSupportDetails const &swapchainSupportDetails() const;

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
			void copyBuffer(
					VkBuffer srcBuffer,
					VkBuffer dstBuffer,
					VkDeviceSize size) const;
			void executeSingleTimeCommand(std::function<void(VkCommandBuffer)>) const;

			struct SwapchainSupportDetails {
				VkSurfaceCapabilitiesKHR capabilities;
				std::vector<VkSurfaceFormatKHR> formats;
				std::vector<VkPresentModeKHR> presentModes;
			};

		private:
			Graphics(const char *name);
			void initWindow_();
			void initVulkan_();
			util::Result<void, KError> createInstance_();
			util::Result<void, KError> setupDebugMessenger_();
			void pickPhysicalDevice_();
			bool isDeviceSuitable_(VkPhysicalDevice device);
			bool checkDeviceExtensionSupport_(VkPhysicalDevice device);
			util::Result<void, KError> createLogicalDevice_();
			void createComputeDescriptorSetLayout_();
			void createComputePipeline_();
			void createCommandPool_();
			void createDescriptorPool_();
			void createComputeDescriptorSets_();
			void createComputeCommandBuffers_();
			util::Result<void, KError> createSyncObjects_();
			void createSurface_();
			void recordComputeCommandBuffer_(VkCommandBuffer commandBuffer);
			void createTextureSampler_();
			void createComputeResultTexture_();

			bool checkValidationLayerSupport_();
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
			SwapchainSupportDetails querySwapChainSupport_(VkPhysicalDevice device);
			VkSurfaceFormatKHR chooseSwapSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats);
			VkPresentModeKHR chooseSwapPresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes);
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
			VkDescriptorSetLayout computeDescriptorSetLayout_;
			VkDescriptorPool descriptorPool_;
			std::vector<VkDescriptorSet> computeDescriptorSets_;
			VkPipelineLayout computePipelineLayout_;
			VkPipeline computePipeline_;
			VkCommandPool commandPool_;
			uint32_t mipLevels_;
			VkSampler textureSampler_;
			VkDeviceMemory computeResultMemory_;
			VkImage computeResultImage_;
			ImageView _compute_result_image_view;
			std::vector<VkCommandBuffer> computeCommandBuffers_;
			std::vector<Semaphore> computeFinishedSemaphores_;
			std::vector<Fence> computeInFlightFences_;
			SwapchainSupportDetails swapchainSupportDetails_;

			//imgui stuff
			bool framebufferResized_ = false;
			uint32_t currentFrame_ = 0;
	};
}
