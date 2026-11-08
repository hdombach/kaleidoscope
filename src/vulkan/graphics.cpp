#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <set>
#include <array>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "graphics.hpp"
#include "defs.hpp"
#include "util/log.hpp"
#include "util/BaseError.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/detail/compute_vector_relational.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace vulkan {

	Graphics *Graphics::DEFAULT = nullptr;

	void Graphics::init_default(const char *name) {
		DEFAULT = new Graphics();
		DEFAULT->_init_window();
		DEFAULT->_init_vulkan();
	}

	void Graphics::delete_default() {
		delete DEFAULT;
	}

	Graphics::~Graphics() {
		_destroy();
	}

	void Graphics::wait_idle() const {
		vkDeviceWaitIdle(_device);
	}
	GLFWwindow * Graphics::window() {
		return _window;
	}

	VkSurfaceKHR const &Graphics::surface() const {
		return _surface;
	}
	VkPhysicalDevice const &Graphics::physical_device() const {
		return _physical_device;
	}
	VkDevice const &Graphics::device() const {
		return _device;
	}
	VkInstance const &Graphics::instance() const {
		return _instance;
	}
	Sampler const &Graphics::main_texture_sampler() const {
		return _main_sampler;
	}
	Sampler const &Graphics::near_texture_sampler() const {
		return _near_sampler;
	}
	GLFWwindow* Graphics::window() const {
		return _window;
	}
	VkCommandPool Graphics::command_pool() const {
		return _command_pool;
	}
	VkQueue Graphics::queue() const {
		return _queue;
	}
	Graphics::SwapchainSupportDetails const &Graphics::swapchain_support_details() const {
		return _swapchain_support_details;
	}
	util::Result<uint32_t, Error> Graphics::find_memory_type(
			uint32_t type_filter,
			VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(_physical_device, &mem_properties);

		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
			if ((type_filter & (1 << i))
					&& (mem_properties.memoryTypes[i].propertyFlags) & properties)
			{
				return i;
			}
		}

		return Error(ErrorType::VULKAN, "Invalid memory property");
	}

	VkFormat Graphics::find_supported_format(
			const std::vector<VkFormat> &candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features) const
	{
		return _find_supported_format(candidates, tiling, features);
	}
	VkShaderModule Graphics::create_shader_module(const std::string &code) const {
		return _create_shader_module(code);
	}
	std::optional<uint32_t> Graphics::get_queue_index() const {
		return _get_queue_index(_physical_device);
	}
	void Graphics::create_buffer(
			VkDeviceSize size,
			VkBufferUsageFlags usage,
			VkMemoryPropertyFlags properties,
			VkBuffer &buffer,
			VkDeviceMemory &bufferMemory) const
	{
		_create_buffer(size, usage, properties, buffer, bufferMemory);
	}
	void Graphics::transition_image_layout(
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			uint32_t mipLevels) const
	{
		_transition_image_layout(image, format, oldLayout, newLayout, mipLevels);
	}
	void Graphics::copy_buffer_to_image(
			VkBuffer buffer,
			VkImage image,
			uint32_t width,
			uint32_t height) const
	{
		_copy_buffer_to_image(buffer, image, width, height);
	}
	void Graphics::generate_mipmaps(
			VkImage image,
			VkFormat imageFormat,
			int32_t texWidth,
			int32_t texHeight,
			uint32_t mipLevels) const
	{
		_generate_mipmaps(image, imageFormat, texWidth, texHeight, mipLevels);
	}
	void Graphics::copy_buffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			VkDeviceSize size) const
	{
		_copy_buffer(srcBuffer, dstBuffer, size);
	}
	void Graphics::execute_single_time_command(
				std::function<void (VkCommandBuffer)> command) const
	{
		auto commandBuffer = _begin_single_time_commands();
		command(commandBuffer);
		_end_single_time_commands(commandBuffer);
	
	}

	void Graphics::set_debug_name(VkBuffer buffer, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_BUFFER, buffer, "VkBuffer: " + name);
	}

	void Graphics::set_debug_name(VkImage image, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_IMAGE, image, "VkImage: " + name);
	}

	void Graphics::set_debug_name(VkDeviceMemory memory, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_DEVICE_MEMORY, memory, "VkDeviceMemory: " + name);
	}

	void Graphics::set_debug_name(VkImageView image_view, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_IMAGE_VIEW, image_view, "VkImageView: " + name);
	}

	void Graphics::set_debug_name(VkDescriptorSet descriptor_set, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_DESCRIPTOR_SET, descriptor_set, "VkDescriptorSet: " + name);
	}

	void Graphics::set_debug_name(VkDescriptorPool descriptor_pool, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_DESCRIPTOR_POOL, descriptor_pool, "VkDescriptorPool: " + name);
	}

	void Graphics::set_debug_name(VkSemaphore semaphore, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_SEMAPHORE, semaphore, "VkSemaphore: " + name);
	}

	void Graphics::set_debug_name(VkQueue queue, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_QUEUE, queue, "VkQueue: " + name);
	}

	void Graphics::set_debug_name(VkPipelineLayout pipeline_layout, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_PIPELINE_LAYOUT, pipeline_layout, "VkPipelineLayout: " + name);
	}

	void Graphics::set_debug_name(VkRenderPass render_pass, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_RENDER_PASS, render_pass, "VkRenderPass: " + name);
	}

	void Graphics::set_debug_name(VkShaderModule shader_module, std::string const &name) {
		_set_debug_name(VK_OBJECT_TYPE_SHADER_MODULE, shader_module, "VkShaderModule: " + name);
	}

	void Graphics::_init_window() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		_window = glfwCreateWindow(WIDTH, HEIGHT, "Kaleidoscope", nullptr, nullptr);
		glfwSetWindowUserPointer(_window, this);
		glfwSetFramebufferSizeCallback(_window, _framebuffer_resize_callback);
	}

	void Graphics::_init_vulkan() {
		if (auto err = _create_instance().move_or()) {
			log_fatal_error(err.value());
		}
		log_assert(_instance, "Instance must be initialized");
		_set_obj_name = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
			vkGetInstanceProcAddr(_instance, "vkSetDebugUtilsObjectNameEXT")
		);
		log_assert(_set_obj_name, "Could not setup set_obj_name");
		if (auto err = _setup_debug_messenger().move_or()) {
			log_fatal_error(err.value());
		}
		_create_surface();
		if (auto d = _pick_physical_device()) {
			_physical_device = d.value();
		} else {
			log_error() << "Could not pick physical device" << std::endl;
		}
		if (auto err = _create_logical_device().move_or()) {
			log_fatal_error(err.value());
		}
		_create_command_pool();
		_main_sampler = std::move(Sampler::create_linear().value());
		_near_sampler = std::move(Sampler::create_nearest().value());
		_create_descriptor_pool();
	}

	util::Result<void, Error> Graphics::_create_instance() {
		if (ENABLE_VALIDATION_LAYERS) {
			if (_check_validation_layer_support()) {
				std::cout << "Validation layer enabled" << std::endl;
			} else {
				return Error(ErrorType::VULKAN, "Validation layers requested but no available!");
			}
		}

		auto appInfo = VkApplicationInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = _name;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		auto createInfo = VkInstanceCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = _get_required_extensions();
		extensions.push_back("VK_KHR_get_physical_device_properties2");
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		for (auto &extension : extensions) {
			log_debug() << "Extension: " << extension << std::endl;
		}

		// Should be enabled for macos
		// TODO: smarter lookup
		//createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		auto debugCreateInfo = VkDebugUtilsMessengerCreateInfoEXT{};
		if (ENABLE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

			_populate_debug_messenger_create_info(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VkResult result = vkCreateInstance(&createInfo, nullptr, &_instance);
		if (result != VK_SUCCESS) {
			for (auto &ext : extensions) {
				log_debug() << "extension is " << ext << std::endl;
			}
			return Error(ErrorType::VULKAN, "Could not create vulkan instance", VkError(result));
		}
		log_memory() << "Create instance " << _instance << std::endl;

		return {};
	}

	bool Graphics::_check_validation_layer_support() {
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

	void Graphics::_destroy() {
		log_trace() << "Destroying graphics" << std::endl;
		_main_sampler.destroy();
		_near_sampler.destroy();

		if (_descriptor_pool) {
			vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
			_descriptor_pool = nullptr;
		}

		if (_command_pool) {
			vkDestroyCommandPool(_device, _command_pool, nullptr);
			_command_pool = nullptr;
		}

		if (_device) {
			log_memory() << "destroying device " << _device << std::endl;
			vkDestroyDevice(_device, nullptr);
			_device = nullptr;
		}

		if (ENABLE_VALIDATION_LAYERS) {
			_destroy_debug_utils_messenger_EXT(_instance, _debug_messenger, nullptr);
			_debug_messenger = nullptr;
		}

		log_memory() << "Destroy instance " << _instance << std::endl;
		vkDestroyInstance(_instance, nullptr);
		_instance = nullptr;

		glfwDestroyWindow(_window);
		_window = nullptr;

		glfwTerminate();
	}

	std::vector<const char*> Graphics::_get_required_extensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		//extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		return extensions;
	}
	util::Result<void, Error> Graphics::_setup_debug_messenger() {
		if (!ENABLE_VALIDATION_LAYERS) return {};
		auto createInfo = VkDebugUtilsMessengerCreateInfoEXT{};
		_populate_debug_messenger_create_info(createInfo);
		auto result = _create_debug_utils_messenger_EXT(_instance, &createInfo, nullptr, &_debug_messenger);
		if (result != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create debut utils", VkError(result));
		}
		return {};
	}
	void Graphics::_populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
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

		createInfo.pfnUserCallback = _debug_callback;
	}

	struct StrCmp {
		bool operator()(const char *lhs, const char *rhs) const {
			return strcmp(lhs, rhs);
		}
	};

	std::optional<VkPhysicalDevice> Graphics::_pick_physical_device() {
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(_instance, &device_count, nullptr);

		if (device_count == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		auto devices = std::vector<VkPhysicalDevice>(device_count);
		vkEnumeratePhysicalDevices(_instance, &device_count, devices.data());

		for (const auto& device : devices) {
			uint32_t extension_count = 0;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
			auto extensions = std::vector<VkExtensionProperties>(extension_count);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());

			auto required = std::set<const char *, StrCmp>(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());
			for (auto &extension : extensions) {
				if (required.contains(extension.extensionName)) {
					required.erase(extension.extensionName);
				}
			}

			if (!required.empty()) {
				continue;
			}

			auto surface_capabilities = VkSurfaceCapabilitiesKHR{};
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &surface_capabilities);

			uint32_t format_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, nullptr);
			if (format_count == 0) continue;
			auto surface_formats = std::vector<VkSurfaceFormatKHR>(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &format_count, surface_formats.data());

			uint32_t present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, nullptr);
			if (present_mode_count == 0) continue;
			auto present_modes = std::vector<VkPresentModeKHR>(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &present_mode_count, present_modes.data());

			VkPhysicalDeviceFeatures supported_features;
			vkGetPhysicalDeviceFeatures(device, &supported_features);

			if (!supported_features.samplerAnisotropy) continue;
			return device;
		}

		return std::nullopt;
	}

	util::Result<void, Error> Graphics::_create_logical_device() {
		auto index = _get_queue_index(_physical_device);
		float queuePriority = 1.0f;

		auto queue_create_info = VkDeviceQueueCreateInfo{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = index.value();
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queuePriority;

		auto deviceFeatures = VkPhysicalDeviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.independentBlend = VK_TRUE;
		auto createInfo = VkDeviceCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pQueueCreateInfos = &queue_create_info;
		createInfo.pEnabledFeatures = &deviceFeatures;

		std::vector<const char*> extensions;
		for (auto &e : _available_extensions) {
			if (strcmp(e.extensionName, "VK_KHR_portability_subset") == 0) {
				extensions.push_back("VK_KHR_portability_subset");
			}
		}
		extensions.insert(extensions.end(), REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		auto result = vkCreateDevice(_physical_device, &createInfo, nullptr, &_device);
		if (result != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create device", VkError(result));
		}
		log_memory() << "Created device " << _device << std::endl;
		//TODO: the pipeline objects should probably own graphics queue
		vkGetDeviceQueue(_device, index.value(), 0, &_queue);
		//TODO: search specifically for a compute queue

		set_debug_name(_queue, "Graphics queue");
		return {};
	}

	void Graphics::_create_command_pool() {
		auto index = _get_queue_index(_physical_device);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = index.value();

		if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_command_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	void Graphics::_create_descriptor_pool() {
		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT) * 2;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(FRAMES_IN_FLIGHT);


		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(FRAMES_IN_FLIGHT * 2);
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptor_pool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	std::optional<uint32_t> Graphics::_get_queue_index(VkPhysicalDevice device) const {
		uint32_t property_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &property_count, nullptr);

		auto properties = std::vector<VkQueueFamilyProperties>(property_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &property_count, properties.data());

		int i = 0;
		for (const auto &property : properties) {
			VkBool32 queue_support = property.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);	
			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &present_support);

			if (queue_support && present_support) {
				return i;
			}

			i++;
		}
		return std::nullopt;
	}

	void Graphics::_create_surface() {
		if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	VkShaderModule Graphics::_create_shader_module(const std::string& code) const {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderModule;
	}
	uint32_t Graphics::_find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(_physical_device, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}
	void Graphics::_create_buffer(
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

		if (vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

		auto allocInfo = VkMemoryAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = _find_memory_type(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(_device, buffer, bufferMemory, 0);
	}
	void Graphics::_copy_buffer(
			VkBuffer srcBuffer,
			VkBuffer dstBuffer,
			VkDeviceSize size) const
	{
		VkCommandBuffer commandBuffer = _begin_single_time_commands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		_end_single_time_commands(commandBuffer);
	}

	VkCommandBuffer Graphics::_begin_single_time_commands() const {
		auto allocInfo = VkCommandBufferAllocateInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = _command_pool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}
	void Graphics::_end_single_time_commands(VkCommandBuffer commandBuffer) const {
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(_queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(_queue);

		vkFreeCommandBuffers(_device, _command_pool, 1, &commandBuffer);
	}
	void Graphics::_transition_image_layout(
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			uint32_t mipLevels) const
	{
		auto commandBuffer = _begin_single_time_commands();

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
			if (_has_stencil_component(format)) {
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
		} else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
				newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		} else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
				newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
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

		_end_single_time_commands(commandBuffer);
	}

	void Graphics::_copy_buffer_to_image(
			VkBuffer buffer,
			VkImage image,
			uint32_t width,
			uint32_t height) const
	{
		VkCommandBuffer commandBuffer = _begin_single_time_commands();

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

		_end_single_time_commands(commandBuffer);
	}
	VkFormat Graphics::_find_supported_format(
			const std::vector<VkFormat>& candidates,
			VkImageTiling tiling,
			VkFormatFeatureFlags features) const
	{
		for (auto format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(_physical_device, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}
	VkFormat Graphics::_find_depth_format() {
		return _find_supported_format(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
	bool Graphics::_has_stencil_component(VkFormat format) const {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
	void Graphics::_generate_mipmaps(
			VkImage image,
			VkFormat imageFormat,
			int32_t texWidth,
			int32_t texHeight,
			uint32_t mipLevels) const
	{
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(
				_physical_device,
				imageFormat,
				&formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
			log_fatal_error() << "Texture image format does not support linear blitting" << std::endl;
		}

		auto commandBuffer = _begin_single_time_commands();

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

		_end_single_time_commands(commandBuffer);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Graphics::_debug_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void* pUserData
	) {

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			log_error() << pCallbackData->pMessage << std::endl;
		} else {
			log_warning() << pCallbackData->pMessage << std::endl;
		}

		// Documentation says to always return false
		return VK_FALSE;
	}

	void Graphics::_set_debug_name(VkObjectType type, void *addr, std::string const &name) {
		log_assert(_set_obj_name, "_set_obj_name must be set up");

		auto name_info = VkDebugUtilsObjectNameInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = nullptr,
			.objectType = type,
			.objectHandle = reinterpret_cast<uint64_t>(addr),
			.pObjectName = name.c_str()
		};

		auto res = _set_obj_name(_device, &name_info);
		if (res != VK_SUCCESS) {
			log_error() << "Could not set debug name " << name
				<< " for object of type " << type << std::endl;
		}
	}

	void Graphics::_destroy_debug_utils_messenger_EXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(_instance, debugMessenger, pAllocator);
		}
	}

	void Graphics::_framebuffer_resize_callback(GLFWwindow* window, int width, int height) {

	}

	VkResult Graphics::_create_debug_utils_messenger_EXT(
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
