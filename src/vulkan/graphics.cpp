#include "graphics.h"
#include "defs.h"
#include "error.h"
#include "file.h"
#include "log.h"
#include "uniformBufferObject.h"
#include "vertex.h"
#include "vulkan/vulkan_core.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <glm/detail/compute_vector_relational.hpp>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <set>
#include <unordered_map>
#include <strstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace vulkan {
	Graphics::Graphics(const char *name) {
		initWindow_();
		initVulkan_();
	}

	Graphics::Graphics(Graphics&& old) {
		name_ = old.name_;
		window_ = old.window_;
		instance_ = old.instance_;
		debugMessenger_ = old.debugMessenger_;
		physicalDevice_ = old.physicalDevice_;
		device_ = old.device_;
		graphicsQueue_ = old.graphicsQueue_;
		presentQueue_ = old.presentQueue_;
		computeQueue_ = old.computeQueue_;
		surface_ = old.surface_;
		computeDescriptorSetLayout_ = old.computeDescriptorSetLayout_;
		descriptorPool_ = old.descriptorPool_;
		computeDescriptorSets_ = std::move(old.computeDescriptorSets_);
		computePipelineLayout_ = old.computePipelineLayout_;
		computePipeline_ = old.computePipeline_;
		commandPool_= old.commandPool_;
		mipLevels_ = old.mipLevels_;
		textureSampler_ = old.textureSampler_;
		computeResultMemory_ = old.computeResultMemory_;
		computeResultImage_ = old.computeResultImage_;
		computeResultImageView_ = old.computeResultImageView_;
		computeCommandBuffers_ = std::move(old.computeCommandBuffers_);
		computeFinishedSemaphores_ = std::move(old.computeFinishedSemaphores_);
		computeInFlightFences_ = std::move(old.computeInFlightFences_);
		framebufferResized_ = old.framebufferResized_;
		currentFrame_ = old.currentFrame_;
		swapchainSupportDetails_ = old.swapchainSupportDetails_;

		glfwSetWindowUserPointer(window_, this);

	}

	Graphics& Graphics::operator=(Graphics &&old) {
		name_ = old.name_;
		window_ = old.window_;
		instance_ = old.instance_;
		debugMessenger_ = old.debugMessenger_;
		physicalDevice_ = old.physicalDevice_;
		device_ = old.device_;
		graphicsQueue_ = old.graphicsQueue_;
		presentQueue_ = old.presentQueue_;
		computeQueue_ = old.computeQueue_;
		surface_ = old.surface_;
		computeDescriptorSetLayout_ = old.computeDescriptorSetLayout_;
		descriptorPool_ = old.descriptorPool_;
		computeDescriptorSets_ = std::move(old.computeDescriptorSets_);
		computePipelineLayout_ = old.computePipelineLayout_;
		computePipeline_ = old.computePipeline_;
		commandPool_= old.commandPool_;
		mipLevels_ = old.mipLevels_;
		textureSampler_ = old.textureSampler_;
		computeResultMemory_ = old.computeResultMemory_;
		computeResultImage_ = old.computeResultImage_;
		computeResultImageView_ = old.computeResultImageView_;
		computeCommandBuffers_ = std::move(old.computeCommandBuffers_);
		computeFinishedSemaphores_ = std::move(old.computeFinishedSemaphores_);
		computeInFlightFences_ = std::move(old.computeInFlightFences_);
		framebufferResized_ = old.framebufferResized_;
		currentFrame_ = old.currentFrame_;
		swapchainSupportDetails_ = old.swapchainSupportDetails_;

		glfwSetWindowUserPointer(window_, this);

		return *this;
	}

	Graphics::~Graphics() {
		cleanup_();
	}

	void Graphics::waitIdle() const {
		vkDeviceWaitIdle(device_);
	}
	GLFWwindow * Graphics::window() {
		return window_;
	}

	VkSurfaceKHR const &Graphics::surface() const {
		return surface_;
	}
	VkPhysicalDevice const &Graphics::physicalDevice() const {
		return physicalDevice_;
	}
	VkDevice const &Graphics::device() const {
		return device_;
	}
	VkInstance const &Graphics::instance() const {
		return instance_;
	}
	VkSampler Graphics::mainTextureSampler() const {
		return textureSampler_;
	}
	GLFWwindow* Graphics::window() const {
		return window_;
	}
	VkCommandPool Graphics::commandPool() const {
		return commandPool_;
	}
	VkQueue Graphics::graphicsQueue() const {
		return graphicsQueue_;
	}
	VkQueue Graphics::presentQueue() const {
		return presentQueue_;
	}
	VkImageView Graphics::computeImageView() const {
		return computeResultImageView_;
	}
	Graphics::SwapchainSupportDetails const &Graphics::swapchainSupportDetails() const {
		return swapchainSupportDetails_;
	}

	VkFormat Graphics::findSupportedFormat(
			const std::vector<VkFormat> &candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features) const
	{
		return findSupportedFormat_(candidates, tiling, features);
	}
	VkShaderModule Graphics::createShaderModule(const std::string &code) const {
		return createShaderModule_(code);
	}
	QueueFamilyIndices Graphics::findQueueFamilies() const {
		return findQueueFamilies_(physicalDevice_);
	}
	void Graphics::createBuffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer &buffer,
			VkDeviceMemory &bufferMemory) const
	{
		createBuffer_(size, usage, properties, buffer, bufferMemory);
	}
	void Graphics::createImage(
			uint32_t width,
			uint32_t height,
			uint32_t mipLevels,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkImage &image,
			VkDeviceMemory &imageMemory) const
	{
		createImage_(width, height, mipLevels, format, tiling, usage, properties, image, imageMemory);
	}
	void Graphics::transitionImageLayout(
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			uint32_t mipLevels) const
	{
		transitionImageLayout_(image, format, oldLayout, newLayout, mipLevels);
	}
	void Graphics::copyBufferToImage(
			VkBuffer buffer,
			VkImage image,
			uint32_t width,
			uint32_t height) const
	{
		copyBufferToImage_(buffer, image, width, height);
	}
	void Graphics::generateMipmaps(
			VkImage image,
			VkFormat imageFormat,
			int32_t texWidth,
			int32_t texHeight,
			uint32_t mipLevels) const
	{
		generateMipmaps_(image, imageFormat, texWidth, texHeight, mipLevels);
	}
	VkImageView Graphics::createImageView(
			VkImage image,
			VkFormat format,
			VkImageAspectFlags aspectFlags,
			uint32_t mipLevels) const
	{
		return createImageView_(image, format, aspectFlags, mipLevels);
	}
	void Graphics::copyBuffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			VkDeviceSize size) const
	{
		copyBuffer_(srcBuffer, dstBuffer, size);
	}
	void Graphics::executeSingleTimeCommand(
				std::function<void (VkCommandBuffer)> command) const
	{
		auto commandBuffer = beginSingleTimeCommands_();
		command(commandBuffer);
		endSingleTimeCommands_(commandBuffer);
	}

	void Graphics::initWindow_() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window_ = glfwCreateWindow(WIDTH, HEIGHT, "Kaleidoscope", nullptr, nullptr);
		glfwSetWindowUserPointer(window_, this);
		glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback_);
	}

	void Graphics::initVulkan_() {
		createInstance_();
		setupDebugMessenger_();
		createSurface_();
		pickPhysicalDevice_();
		createLogicalDevice_();
		createCommandPool_();
		createComputeDescriptorSetLayout_();
		createComputePipeline_();
		createTextureSampler_();
		createComputeResultTexture_();
		createDescriptorPool_();
		createComputeDescriptorSets_();
		createComputeCommandBuffers_();
		createSyncObjects_();
	}

	void Graphics::createInstance_() {
		if (ENABLE_VALIDATION_LAYERS) {
			if (checkValidationLayerSupport_()) {
				std::cout << "Validation layer enabled" << std::endl;
			} else {
				throw std::runtime_error("validation layers requrested but not available!");
			}
		}

		auto appInfo = VkApplicationInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = name_;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		auto createInfo = VkInstanceCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions_();
		extensions.push_back("VK_KHR_get_physical_device_properties2");
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		auto debugCreateInfo = VkDebugUtilsMessengerCreateInfoEXT{};
		if (ENABLE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			populateDebugMessengerCreateInfo_(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	bool Graphics::checkValidationLayerSupport_() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (auto layerName: VALIDATION_LAYERS) {
			bool layerFound = false;
			for (auto layerProperties: availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}
			if (!layerFound) {
				return false;
			}
		}
		return true;
	}

	void Graphics::cleanup_() {
		vkDestroySampler(device_, textureSampler_, nullptr);
		vkDestroyImageView(device_, computeResultImageView_, nullptr);
		vkDestroyImage(device_, computeResultImage_, nullptr);
		vkFreeMemory(device_, computeResultMemory_, nullptr);

		vkDestroyDescriptorSetLayout(device_, computeDescriptorSetLayout_, nullptr);
		vkFreeDescriptorSets(device_, descriptorPool_, computeDescriptorSets_.size(), computeDescriptorSets_.data());

		vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

		vkDestroyPipeline(device_, computePipeline_, nullptr);
		vkDestroyPipelineLayout(device_, computePipelineLayout_, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device_, computeFinishedSemaphores_[i], nullptr);
			vkDestroyFence(device_, computeInFlightFences_[i], nullptr);
		}

		vkDestroyCommandPool(device_, commandPool_, nullptr);

		vkDestroyDevice(device_, nullptr);

		if (ENABLE_VALIDATION_LAYERS) {
			destroyDebugUtilsMessengerEXT_(instance_, debugMessenger_, nullptr);
		}

		vkDestroyInstance(instance_, nullptr);

		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	std::vector<const char*> Graphics::getRequiredExtensions_() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (ENABLE_VALIDATION_LAYERS) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		return extensions;
	}
	void Graphics::setupDebugMessenger_() {
		if (!ENABLE_VALIDATION_LAYERS) return;
		auto createInfo = VkDebugUtilsMessengerCreateInfoEXT{};
		populateDebugMessengerCreateInfo_(createInfo);
		auto result = createDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}
	void Graphics::populateDebugMessengerCreateInfo_(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo.pfnUserCallback = debugCallback;
	}
	void Graphics::pickPhysicalDevice_() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		auto devices = std::vector<VkPhysicalDevice>(deviceCount);
		vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

		for (const auto& device : devices) {
			if (isDeviceSuitable_(device)) {
				physicalDevice_ = device;
				break;
			}
		}

		if (physicalDevice_ == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	bool Graphics::isDeviceSuitable_(VkPhysicalDevice device) {
		auto indices = findQueueFamilies_(device);
		bool extensionsSupported = checkDeviceExtensionSupport_(device);
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			swapchainSupportDetails_ = querySwapChainSupport_(device);
			swapChainAdequate = !swapchainSupportDetails_.formats.empty() && !swapchainSupportDetails_.presentModes.empty();
		} else {
			util::log_error("Extensions are not supported");
		}
		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
		return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}
	bool Graphics::checkDeviceExtensionSupport_(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		auto availableExtensions = std::vector<VkExtensionProperties>(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		auto requiredExtensions = std::set<std::string>(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
		for (const auto& extension: availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}
	void Graphics::createLogicalDevice_() {
		auto indices = findQueueFamilies_(physicalDevice_);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		auto uniqueQueueFamilies = std::set<uint32_t>{indices.graphicsFamily.value(), indices.presentFamily.value()};
		float queuePriority = 1.0f;
		for (auto queueFamily : uniqueQueueFamilies) {
			auto queueCreateInfo = VkDeviceQueueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		auto deviceFeatures = VkPhysicalDeviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		auto createInfo = VkDeviceCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

		std::vector<const char*> extensions;
		extensions.push_back("VK_KHR_portability_subset");
		extensions.insert(extensions.end(), DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		if (ENABLE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}
		auto result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
		//TODO: the pipeline objects should probably own graphics queue
		vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
		//TODO: search specifically for a compute queue
		vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &computeQueue_);
		vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
	}

	void Graphics::createComputePipeline_() {
		auto computeShaderCode = util::readEnvFile("src/shaders/default_shader.comp.spv");

		auto computeShaderModule = createShaderModule_(computeShaderCode);

		auto computeShaderStageInfo = VkPipelineShaderStageCreateInfo{};
		computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		computeShaderStageInfo.module = computeShaderModule;
		computeShaderStageInfo.pName = "main";

		auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout_;

		require(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &computePipelineLayout_));

		auto pipelineInfo = VkComputePipelineCreateInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.layout = computePipelineLayout_;
		pipelineInfo.stage = computeShaderStageInfo;

		require(vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline_));

		vkDestroyShaderModule(device_, computeShaderModule, nullptr);
	}
	void Graphics::createCommandPool_() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies_(physicalDevice_);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	void Graphics::createDescriptorPool_() {
		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2);
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}
	void Graphics::createComputeDescriptorSets_() {
		auto layouts = std::vector<VkDescriptorSetLayout>(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout_);
		auto allocInfo = VkDescriptorSetAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool_;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		computeDescriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
		require(vkAllocateDescriptorSets(device_, &allocInfo, computeDescriptorSets_.data()));

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			auto imageInfo = VkDescriptorImageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageInfo.imageView = computeResultImageView_;


			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = computeDescriptorSets_[i];
			descriptorWrites[0].dstBinding = 2;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(
					device_,
					static_cast<uint32_t>(descriptorWrites.size()),
					descriptorWrites.data(),
					0,
					nullptr);
		}
	}
	void Graphics::createComputeDescriptorSetLayout_() {
		auto layoutBindings = std::array<VkDescriptorSetLayoutBinding, 1>{};
		layoutBindings[0].binding = 2;
		layoutBindings[0].descriptorCount = 1;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		layoutBindings[0].pImmutableSamplers = nullptr;
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		auto layoutInfo = VkDescriptorSetLayoutCreateInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		require(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &computeDescriptorSetLayout_));
	}
	void Graphics::createComputeCommandBuffers_() {
		computeCommandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

		auto allocInfo = VkCommandBufferAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool_;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) computeCommandBuffers_.size();

		require(vkAllocateCommandBuffers(device_, &allocInfo, computeCommandBuffers_.data()));
	}
	void Graphics::createSyncObjects_() {
		computeFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		computeInFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			require(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &computeFinishedSemaphores_[i]));
			require(vkCreateFence(device_, &fenceInfo, nullptr, &computeInFlightFences_[i]));
		}
	}

	void Graphics::recordComputeCommandBuffer_(VkCommandBuffer commandBuffer) {
		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		require(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline_);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout_, 0, 1, &computeDescriptorSets_[currentFrame_], 0, nullptr);

		//vkCmdDispatch(commandBuffer, mainRenderPipeline_->swapchainExtent().width, mainRenderPipeline_->swapchainExtent().height, 1);
		vkCmdDispatch(commandBuffer, 10, 10, 1);


		require(vkEndCommandBuffer(commandBuffer));
	}
	void Graphics::createTextureSampler_() {
		auto samplerInfo = VkSamplerCreateInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		auto properties = VkPhysicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0;
		samplerInfo.maxLod = static_cast<float>(mipLevels_);
		samplerInfo.mipLodBias = 0.0f;

		require(vkCreateSampler(device_, &samplerInfo, nullptr, &textureSampler_));
	}

	void Graphics::createComputeResultTexture_() {
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
		createImage_(
				100, //width
				100, //height
				1,
				imageFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				computeResultImage_,
				computeResultMemory_);

		computeResultImageView_ = createImageView_(
				computeResultImage_,
				imageFormat,
				VK_IMAGE_ASPECT_COLOR_BIT,
				1);
		transitionImageLayout_(
				computeResultImage_,
				imageFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL,
				1);
	}

	
	QueueFamilyIndices Graphics::findQueueFamilies_(VkPhysicalDevice device) const {
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);

			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	Graphics::SwapchainSupportDetails Graphics::querySwapChainSupport_(VkPhysicalDevice device) {
		auto details = SwapchainSupportDetails{};

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR Graphics::chooseSwapSurfaceFormat_(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB &&
					availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR Graphics::chooseSwapPresentMode_(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void Graphics::createSurface_() {
		if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	VkShaderModule Graphics::createShaderModule_(const std::string& code) const {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}
	uint32_t Graphics::findMemoryType_(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}
	void Graphics::createBuffer_(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer& buffer,
			VkDeviceMemory& bufferMemory) const
	{
		auto bufferInfo = VkBufferCreateInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

		auto allocInfo = VkMemoryAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType_(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device_, buffer, bufferMemory, 0);
	}
	void Graphics::copyBuffer_(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			VkDeviceSize size) const
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands_();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands_(commandBuffer);
	}

	void Graphics::createImage_(
			uint32_t width,
			uint32_t height,
			uint32_t mipLevels,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkImage &image,
			VkDeviceMemory &imageMemory) const
	{
		auto imageInfo = VkImageCreateInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.mipLevels = mipLevels;

		if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device_, image, &memRequirements);

		auto allocInfo = VkMemoryAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType_(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocated image memory!");
		}

		vkBindImageMemory(device_, image, imageMemory, 0);
	}
	VkCommandBuffer Graphics::beginSingleTimeCommands_() const {
		auto allocInfo = VkCommandBufferAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool_;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}
	void Graphics::endSingleTimeCommands_(VkCommandBuffer commandBuffer) const {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue_);

		vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
	}
	void Graphics::transitionImageLayout_(
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			uint32_t mipLevels) const
	{
		auto commandBuffer = beginSingleTimeCommands_();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (hasStencilComponent_(format)) {
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		} else {
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
				newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
				newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
				newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
				newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else {
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

		endSingleTimeCommands_(commandBuffer);
	}

	void Graphics::copyBufferToImage_(
			VkBuffer buffer,
			VkImage image,
			uint32_t width,
			uint32_t height) const
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands_();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(
				commandBuffer,
				buffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region);

		endSingleTimeCommands_(commandBuffer);
	}
	VkImageView Graphics::createImageView_(
			VkImage image,
			VkFormat format,
			VkImageAspectFlags aspectFlags,
			uint32_t mipLevels) const
	{
		auto viewInfo = VkImageViewCreateInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = mipLevels;

		VkImageView imageView;
		require(vkCreateImageView(device_, &viewInfo, nullptr, &imageView));
		return imageView;
	}
	VkFormat Graphics::findSupportedFormat_(
			const std::vector<VkFormat>& candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features) const
	{
		for (auto format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}
	VkFormat Graphics::findDepthFormat_() {
		return findSupportedFormat_(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
	bool Graphics::hasStencilComponent_(VkFormat format) const {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
	void Graphics::generateMipmaps_(
			VkImage image,
			VkFormat imageFormat,
			int32_t texWidth,
			int32_t texHeight,
			uint32_t mipLevels) const
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(
				physicalDevice_,
				imageFormat,
				&formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			util::log_fatal_error("Texture image format does not support linear blitting!");
		}

		auto commandBuffer = beginSingleTimeCommands_();

		auto barrier = VkImageMemoryBarrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&barrier);

			auto blit = VkImageBlit{};
			blit.srcOffsets[0] = {0, 0, 0};
			blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = {0, 0, 0};
			blit.dstOffsets[1] = {
				mipWidth > 1 ? mipWidth / 2 : 1,
				mipHeight > 1 ? mipHeight / 2 : 1,
				1
			};
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(
					commandBuffer,
					image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&blit,
					VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					0,
					nullptr,
					0,
					nullptr,
					1,
					&barrier);
			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier);

		endSingleTimeCommands_(commandBuffer);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Graphics::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void* pUserData)
	{

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT && false) {
			throw std::runtime_error(pCallbackData->pMessage);
		} else {
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		}

		return VK_FALSE;
	}

	void Graphics::destroyDebugUtilsMessengerEXT_(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance_, debugMessenger, pAllocator);
		}
	}

	void Graphics::framebufferResizeCallback_(GLFWwindow* window, int width, int height) {
		auto graphics = reinterpret_cast<Graphics*>(glfwGetWindowUserPointer(window));
	}

	VkResult Graphics::createDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

}
