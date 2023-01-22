#include "Instance.h"
#include "DebugUtilsMessenger.h"
#include "Defs.h"
#include "Error.h"
#include "vulkan/vulkan_core.h"
#include <iostream>

namespace vulkan {
	std::shared_ptr<Instance> Instance::create_shared(VkInstanceCreateInfo &createInfo) {
		return std::shared_ptr<Instance>(new Instance(createInfo));
	}

	std::unique_ptr<Instance> Instance::create_unique(VkInstanceCreateInfo &createInfo) {
		return std::unique_ptr<Instance>(new Instance(createInfo));
	}

	VkInstance& Instance::operator*() {
		return instance;
	}

	Instance::~Instance() {
		vkDestroyInstance(instance, nullptr);
	}

	Instance::Instance(VkInstanceCreateInfo &createInfo) {
		auto result = vkCreateInstance(&createInfo, nullptr, &instance);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	InstanceFactory::InstanceFactory() {
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		debugMessageCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

		createInfo.pApplicationInfo = &appInfo;
	};

	InstanceFactory &InstanceFactory::default_config() {
		if (vulkan::ENABLE_VALIDATION_LAYERS) {
			if (checkValidationLayerSupport()) {
				std::cout << "Validation layer enabled" << std::endl;
			} else {
				throw std::runtime_error("validation layers requrested but no available!");
			}
		}

		appInfo.pApplicationName = "Kaleidoscope";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		loadRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		if (ENABLE_VALIDATION_LAYERS) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(vulkan::VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = vulkan::VALIDATION_LAYERS.data();

			debugMessageCreateInfo = DebugUtilsMessengerFactory().default_config().createInfo();

			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessageCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		return *this;
	}

	SharedInstance InstanceFactory::create_shared() {
		return Instance::create_shared(createInfo);
	}

	UniqueInstance InstanceFactory::create_unique() {
		return Instance::create_unique(createInfo);
	}

	void InstanceFactory::loadRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		requiredExtensions = std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (vulkan::ENABLE_VALIDATION_LAYERS) {
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		requiredExtensions.push_back("VK_KHR_get_physical_device_properties2");
	}

	bool InstanceFactory::checkValidationLayerSupport() {
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
}
