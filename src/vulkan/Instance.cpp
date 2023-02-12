#include "Instance.h"
#include "DebugUtilsMessenger.h"
#include "Defs.h"
#include "Error.h"
#include "vulkan/vulkan_core.h"
#include <iostream>

//			VkInstanceCreateInfo createInfo{};
//			VkApplicationInfo appInfo{};
//			VkDebugUtilsMessengerCreateInfoEXT debugMessageCreateInfo{};
//			std::vector<const char*> requiredExtensions;


namespace vulkan {

	void InstanceDeleter::operator()(VkInstance *instance) const {
		vkDestroyInstance(*instance, nullptr);
		delete instance;
	}

	bool checkValidationLayerSupport() {
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName: vulkan::VALIDATION_LAYERS) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
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

	Instance::Instance(const char *name) {
		if (vulkan::ENABLE_VALIDATION_LAYERS) {
			if (checkValidationLayerSupport()) {
				std::cout << "Validation layer enabled" << std::endl;
			} else {
				throw std::runtime_error("validation layers requrested but no available!");
			}
		}

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = name;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		createInfo.pApplicationInfo = &appInfo;

		//required extensions
		{
			uint32_t glfwExtensionCount = 0;
			const char **glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			requiredExtensions_ = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

			if (vulkan::ENABLE_VALIDATION_LAYERS) {
				requiredExtensions_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			requiredExtensions_.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			requiredExtensions_.push_back("VK_KHR_get_physical_device_properties2");
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions_.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions_.data();
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		VkDebugUtilsMessengerCreateInfoEXT debugMessageCreateInfo{};
		debugMessageCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		if (ENABLE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(vulkan::VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = vulkan::VALIDATION_LAYERS.data();

			debugMessageCreateInfo = DebugUtilsMessenger::defaultConfig();

			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessageCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		auto instance = new VkInstance;
		auto result = vkCreateInstance(&createInfo, nullptr, instance);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		this->reset(instance);
	}

	VkInstance& Instance::raw() {
		return **this;
	}
}
