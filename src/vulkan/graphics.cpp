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
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <set>
#include <unordered_map>
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
	void Graphics::tick() {
		drawUi_();
		drawFrame_();
	}
	void Graphics::waitIdle() {
		vkDeviceWaitIdle(device_);
	}
	GLFWwindow * Graphics::window() {
		return window_;
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
		createSwapChain_();
		createImageViews_();
		createRenderPass_();
		createDescriptorSetLayout_();
		createComputeDescriptorSetLayout_();
		createGraphicsPipeline_();
		createComputePipeline_();
		createCommandPool_();
		createDepthResources_();
		createFramebuffers_();
		createTextureImage_();
		createTextureImageView_();
		createTextureSampler_();
		createComputeResultTexture_();
		loadModel_();
		createVertexBuffer_();
		createIndexBuffer_();
		createUniformBuffers_();
		createDescriptorPool_();
		createDescriptorSets_();
		createComputeDescriptorSets_();
		createCommandBuffers_();
		createComputeCommandBuffers_();
		createSyncObjects_();
		initImgui_();
	}

	void Graphics::recreateSwapChain_() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window_, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window_, &width, &height);
			glfwWaitEvents();
		}
		waitIdle();

		util::log_event("Recreate swap chain");

		cleanupSwapChain_();

		createSwapChain_();
		createImageViews_();
		createDepthResources_();
		createFramebuffers_();
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

	void Graphics::cleanupSwapChain_() {
		vkDestroyImageView(device_, depthImageView_, nullptr);
		vkDestroyImage(device_, depthImage_, nullptr);
		vkFreeMemory(device_, depthImageMemory_, nullptr);

		for (size_t i = 0; i < swapChainFramebuffers_.size(); i++) {
			vkDestroyFramebuffer(device_, swapChainFramebuffers_[i], nullptr);
		}

		for (size_t i = 0; i < swapChainImageViews_.size(); i++) {
			vkDestroyImageView(device_, swapChainImageViews_[i], nullptr);
		}

		vkDestroySwapchainKHR(device_, swapChain_, nullptr);
	}

	void Graphics::cleanup_() {
		cleanupSwapChain_();

		vkDestroySampler(device_, textureSampler_, nullptr);
		vkDestroyImageView(device_, textureImageView_, nullptr);
		vkDestroyImage(device_, textureImage_, nullptr);
		vkFreeMemory(device_, textureImageMemory_, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);
			vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);
		}

		vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
		vkDestroyDescriptorPool(device_, imguiPool_, nullptr);
		ImGui_ImplVulkan_Shutdown();

		vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
		vkDestroyDescriptorSetLayout(device_, computeDescriptorSetLayout_, nullptr);

		vkDestroyBuffer(device_, vertexBuffer_, nullptr);
		vkFreeMemory(device_, vertexBufferMemory_, nullptr);

		vkDestroyBuffer(device_, vertexBuffer_, nullptr);
		vkFreeMemory(device_, vertexBufferMemory_, nullptr);

		vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
		vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
	
		vkDestroyPipeline(device_, computePipeline_, nullptr);
		vkDestroyPipelineLayout(device_, computePipelineLayout_, nullptr);

		vkDestroyRenderPass(device_, renderPass_, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
			vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
			vkDestroyFence(device_, inFlightFences_[i], nullptr);

			vkDestroySemaphore(device_, computeFinishedSemaphores_[i], nullptr);
			vkDestroyFence(device_, inFlightFences_[i], nullptr);
		}

		vkDestroyCommandPool(device_, commandPool_, nullptr);

		vkDestroyDevice(device_, nullptr);

		if (ENABLE_VALIDATION_LAYERS) {
			destroyDebugUtilsMessengerEXT_(instance_, debugMessenger_, nullptr);
		}

		vkDestroySurfaceKHR(instance_, surface_, nullptr);
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
			auto swapChainSupport = querySwapChainSupport_(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
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
		vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
		//TODO: search specifically for a compute queue
		vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &computeQueue_);
		vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
	}
	void Graphics::createSwapChain_() {
		auto swapChainSupport = querySwapChainSupport_(physicalDevice_);
		auto surfaceFormat = chooseSwapSurfaceFormat_(swapChainSupport.formats);
		auto presentMode = chooseSwapPresentMode_(swapChainSupport.presentModes);
		auto extent = chooseSwapExtent_(swapChainSupport.capabilities);
		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		auto createInfo = VkSwapchainCreateInfoKHR{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface_;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		auto indices = findQueueFamilies_(physicalDevice_);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		auto result = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
		vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
		swapChainImages_.resize(imageCount);
		vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

		swapChainImageFormat_ = surfaceFormat.format;
		swapChainExtent_ = extent;
	}

	void Graphics::createImageViews_() {
		swapChainImageViews_.resize(swapChainImages_.size());

		for (uint32_t i = 0; i < swapChainImages_.size(); ++i) {
			swapChainImageViews_[i] = createImageView_(swapChainImages_[i], swapChainImageFormat_, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
	}
	void Graphics::createRenderPass_() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat_;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		auto depthAttachment = VkAttachmentDescription{};
		depthAttachment.format = findDepthFormat_();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		auto depthAttachmentRef = VkAttachmentReference{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 2> attachments = {
			colorAttachment,
			depthAttachment};

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		auto result = vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}
	void Graphics::createDescriptorSetLayout_() {
		auto uboLayoutBinding = VkDescriptorSetLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		auto samplerLayoutBinding = VkDescriptorSetLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		auto computeSamperLayoutBinding = VkDescriptorSetLayoutBinding{};
		computeSamperLayoutBinding.binding = 2;
		computeSamperLayoutBinding.descriptorCount = 1;
		computeSamperLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		computeSamperLayoutBinding.pImmutableSamplers = nullptr;
		computeSamperLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings = {uboLayoutBinding, samplerLayoutBinding, computeSamperLayoutBinding};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor set layout!");
		}
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
	void Graphics::createGraphicsPipeline_() {
		auto vertShaderCode = util::readEnvFile("src/shaders/default_shader.vert.spv");
		auto fragShaderCode = util::readEnvFile("src/shaders/default_shader.frag.spv");

		VkShaderModule vertShaderModule = createShaderModule_(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule_(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapChainExtent_.width;
		viewport.height = (float) swapChainExtent_.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent_;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		//uniforms
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		auto result = vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		auto depthStencil = VkPipelineDepthStencilStateCreateInfo{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout_;
		pipelineInfo.renderPass = renderPass_;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		vkDestroyShaderModule(device_, fragShaderModule, nullptr);
		vkDestroyShaderModule(device_, vertShaderModule, nullptr);
	}
	void Graphics::createFramebuffers_() {
		swapChainFramebuffers_.resize(swapChainImageViews_.size());

		for (size_t i = 0; i < swapChainImageViews_.size(); i++) {
			std::array<VkImageView, 2> attachments = {
				swapChainImageViews_[i],
				depthImageView_
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass_;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent_.width;
			framebufferInfo.height = swapChainExtent_.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
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
	void Graphics::createTextureImage_() {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;
		mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer_(
				imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory);

		void *data;
		vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device_, stagingBufferMemory);
		stbi_image_free(pixels);

		createImage_(
				texWidth,
				texHeight,
				mipLevels_,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				textureImage_,
				textureImageMemory_);

		transitionImageLayout_(
				textureImage_,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				mipLevels_);
		copyBufferToImage_(
				stagingBuffer,
				textureImage_,
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(device_, stagingBuffer, nullptr);
		vkFreeMemory(device_, stagingBufferMemory, nullptr);

		generateMipmaps_(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels_);
	}
	void Graphics::loadModel_() {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
			util::log_error(warn + err);
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto& shape : shapes) {
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};

				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0 - attrib.texcoords[2 * index.texcoord_index + 1],
				};

				vertex.color = {1.0f, 1.0f, 1.0f};

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices_.size());
					vertices_.push_back(vertex);
				}

				indices_.push_back(uniqueVertices[vertex]);
			}
		}
	}
	void Graphics::createVertexBuffer_() {
		VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer_(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory);

		void *data;
		vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices_.data(), (size_t) bufferSize);
		vkUnmapMemory(device_, stagingBufferMemory);

		createBuffer_(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBuffer_,
				vertexBufferMemory_);

		copyBuffer_(stagingBuffer, vertexBuffer_, bufferSize);

		vkDestroyBuffer(device_, stagingBuffer, nullptr);
		vkFreeMemory(device_, stagingBufferMemory, nullptr);
	}
	void Graphics::createIndexBuffer_() {
		VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

		VkBuffer staginBuffer;
		VkDeviceMemory staginBufferMemory;
		createBuffer_(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				staginBuffer,
				staginBufferMemory);

		void *data;
		vkMapMemory(device_, staginBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices_.data(), (size_t) bufferSize);
		vkUnmapMemory(device_, staginBufferMemory);

		createBuffer_(
				bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBuffer_,
				indexBufferMemory_);

		copyBuffer_(staginBuffer, indexBuffer_, bufferSize);

		vkDestroyBuffer(device_, staginBuffer, nullptr);
		vkFreeMemory(device_, staginBufferMemory, nullptr);
	}
	void Graphics::createUniformBuffers_() {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory_.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped_.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			createBuffer_(
					bufferSize,
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					uniformBuffers_[i],
					uniformBuffersMemory_[i]);

			require(vkMapMemory(
					device_,
					uniformBuffersMemory_[i],
					0,
					bufferSize,
					0,
					&uniformBuffersMapped_[i]));
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

		if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}
	void Graphics::createDescriptorSets_() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout_);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool_;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets_.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(device_, &allocInfo, descriptorSets_.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers_[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			auto imageInfo = VkDescriptorImageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView_;
			imageInfo.sampler = textureSampler_;

			auto computeImageInfo = VkDescriptorImageInfo{};
			computeImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			computeImageInfo.imageView = computeResultImageView_;
			computeImageInfo.sampler = textureSampler_;

			std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets_[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets_[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSets_[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &computeImageInfo;


			vkUpdateDescriptorSets(
					device_,
					static_cast<uint32_t>(descriptorWrites.size()),
					descriptorWrites.data(),
					0,
					nullptr);
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
	void Graphics::createCommandBuffers_() {
		commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool_;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers_.size();

		if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
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
		imageAvailableSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		computeFinishedSemaphores_.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);
		computeInFlightFences_.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			require(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]));
			require(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]));
			require(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]));

			require(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &computeFinishedSemaphores_[i]));
			require(vkCreateFence(device_, &fenceInfo, nullptr, &computeInFlightFences_[i]));
		}
	}

	void Graphics::recordCommandBuffer_(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass_;
		renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent_;

		auto clearValues = std::array<VkClearValue, 2>{};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		clearValues[1].depthStencil = {1.0f, 0};

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent_.width);
		viewport.height = static_cast<float>(swapChainExtent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent_;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {vertexBuffer_};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout_,
				0,
				1,
				&descriptorSets_[currentFrame_],
				0,
				nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		vkCmdEndRenderPass(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
	void Graphics::recordComputeCommandBuffer_(VkCommandBuffer commandBuffer) {
		auto beginInfo = VkCommandBufferBeginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		require(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline_);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout_, 0, 1, &computeDescriptorSets_[currentFrame_], 0, nullptr);

		vkCmdDispatch(commandBuffer, swapChainExtent_.width, swapChainExtent_.height, 1);

		require(vkEndCommandBuffer(commandBuffer));
	}
	void Graphics::createTextureImageView_() {
		textureImageView_ = createImageView_(textureImage_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels_);
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
	void Graphics::createDepthResources_() {
		VkFormat depthFormat = findDepthFormat_();
		createImage_(
				swapChainExtent_.width,
				swapChainExtent_.height,
				1,
				depthFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				depthImage_,
				depthImageMemory_);
		depthImageView_ = createImageView_(depthImage_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		transitionImageLayout_(
				depthImage_,
				depthFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				1);
	}

	void Graphics::createComputeResultTexture_() {
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
		createImage_(
				swapChainExtent_.width,
				swapChainExtent_.height,
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

	void Graphics::drawFrame_() {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		//Compute submission
		vkWaitForFences(device_, 1, &computeInFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

		//update uniforms here maybe

		vkResetFences(device_, 1, &computeInFlightFences_[currentFrame_]);

		vkResetCommandBuffer(computeCommandBuffers_[currentFrame_], 0);
		recordComputeCommandBuffer_(computeCommandBuffers_[currentFrame_]);

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &computeCommandBuffers_[currentFrame_];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &computeFinishedSemaphores_[currentFrame_];

		require(vkQueueSubmit(computeQueue_, 1, &submitInfo, computeInFlightFences_[currentFrame_]));

		//Main render pass submission
		vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		auto result = vkAcquireNextImageKHR(
				device_,
				swapChain_,
				UINT64_MAX,
				imageAvailableSemaphores_[currentFrame_],
				VK_NULL_HANDLE,
				&imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain_();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw vulkan::Error(result);
		}

		updateUniformBuffer_(currentFrame_);

		vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

		vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);

		ImGui::Render();

		recordCommandBuffer_(commandBuffers_[currentFrame_], imageIndex);

		VkSemaphore waitSemaphores[] = {computeFinishedSemaphores_[currentFrame_], imageAvailableSemaphores_[currentFrame_]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 2;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers_[currentFrame_];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderFinishedSemaphores_[currentFrame_];

		if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphores_[currentFrame_];

		VkSwapchainKHR swapChains[] = {swapChain_};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		result = vkQueuePresentKHR(presentQueue_, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
			framebufferResized_ = false;
			recreateSwapChain_();
		} else if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	
	void Graphics::drawUi_() {
		glfwPollEvents();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();
	}

	void Graphics::updateUniformBuffer_(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent_.width / (float) swapChainExtent_.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped_[currentImage], &ubo, sizeof(ubo));
	}

	QueueFamilyIndices Graphics::findQueueFamilies_(VkPhysicalDevice device) {
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

	SwapChainSupportDetails_ Graphics::querySwapChainSupport_(VkPhysicalDevice device) {
		SwapChainSupportDetails_ details;

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
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
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

	VkExtent2D Graphics::chooseSwapExtent_(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			int width, height;
			glfwGetFramebufferSize(window_, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(
					actualExtent.width,
					capabilities.minImageExtent.width,
					capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(
					actualExtent.height,
					capabilities.minImageExtent.height,
					capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	void Graphics::createSurface_() {
		if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	VkShaderModule Graphics::createShaderModule_(const std::string& code) {
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
	uint32_t Graphics::findMemoryType_(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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
			VkDeviceMemory& bufferMemory)
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
			VkDeviceSize size)
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
			VkDeviceMemory &imageMemory)
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
	VkCommandBuffer Graphics::beginSingleTimeCommands_() {
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
	void Graphics::endSingleTimeCommands_(VkCommandBuffer commandBuffer) {
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
			uint32_t mipLevels)
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
			uint32_t height)
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
			uint32_t mipLevels)
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
			VkFormatFeatureFlags features)
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
	bool Graphics::hasStencilComponent_(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
	void Graphics::generateMipmaps_(
			VkImage image,
			VkFormat imageFormat,
			int32_t texWidth,
			int32_t texHeight,
			uint32_t mipLevels)
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

		for (uint32_t i = 1; i < mipLevels_; i++) {
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

		barrier.subresourceRange.baseMipLevel = mipLevels_ - 1;
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

	void Graphics::initImgui_() {
		auto poolSizes = std::array<VkDescriptorPoolSize, 11>{{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		}};

		auto poolInfo = VkDescriptorPoolCreateInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000;
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();

		require(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &imguiPool_));

		ImGui::CreateContext();

		ImGui_ImplGlfw_InitForVulkan(window_, true);

		auto initInfo = ImGui_ImplVulkan_InitInfo{};
		initInfo.Instance = instance_;
		initInfo.PhysicalDevice = physicalDevice_;
		initInfo.Device = device_;
		initInfo.Queue = graphicsQueue_;
		initInfo.DescriptorPool = imguiPool_;
		initInfo.MinImageCount = 3;
		initInfo.ImageCount = 3;
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&initInfo, renderPass_);

		auto command = beginSingleTimeCommands_();
		ImGui_ImplVulkan_CreateFontsTexture(command);
		endSingleTimeCommands_(command);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Graphics::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void* pUserData)
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

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
		graphics->framebufferResized_ = true;
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
