#include "DebugUtilsMessenger.h"
#include "Error.h"
#include "Instance.h"
#include "vulkan/vulkan_core.h"
#include <iostream>

namespace vulkan {
	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
			void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	SharedDebugUtilsMessenger DebugUtilsMessenger::createShared(VkDebugUtilsMessengerCreateInfoEXT &createInfo, SharedInstance instance) {
		return SharedDebugUtilsMessenger(new DebugUtilsMessenger(createInfo, instance));
	}

	UniqueDebugUtilsMessenger DebugUtilsMessenger::createUnique(VkDebugUtilsMessengerCreateInfoEXT &createInfo, SharedInstance instance) {
		return UniqueDebugUtilsMessenger(new DebugUtilsMessenger(createInfo, instance));
	}

	VkDebugUtilsMessengerEXT& DebugUtilsMessenger::operator*() {
		return debugUtilsMessenger_;
	}

	VkDebugUtilsMessengerEXT& DebugUtilsMessenger::raw() {
		return debugUtilsMessenger_;
	}

	DebugUtilsMessenger::~DebugUtilsMessenger() {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(**instance_, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(**instance_, debugUtilsMessenger_, nullptr);
		}
	}

	DebugUtilsMessenger::DebugUtilsMessenger(VkDebugUtilsMessengerCreateInfoEXT &createInfo, SharedInstance instance): instance_(instance) {
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(**instance_, "vkCreateDebugUtilsMessengerEXT");
		VkResult result;
		if (func != nullptr) {
			result = func(**instance_, &createInfo, nullptr, &debugUtilsMessenger_);
		} else {
			result = VK_ERROR_EXTENSION_NOT_PRESENT;
		}
		
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	/**** Factory ****/

	DebugUtilsMessengerFactory::DebugUtilsMessengerFactory() {
		createInfo_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	}

	DebugUtilsMessengerFactory::DebugUtilsMessengerFactory(SharedInstance instance): instance_(instance) {
		createInfo_.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	}

	DebugUtilsMessengerFactory &DebugUtilsMessengerFactory::default_config() {
		createInfo_.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		createInfo_.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		createInfo_.pfnUserCallback = debugCallback;

		createInfo_.pNext = nullptr;

		return *this;
	}

	SharedDebugUtilsMessenger DebugUtilsMessengerFactory::createShared() {
		return DebugUtilsMessenger::createShared(createInfo_, instance_);
	}

	UniqueDebugUtilsMessenger DebugUtilsMessengerFactory::createUnique() {
		return DebugUtilsMessenger::createUnique(createInfo_, instance_);
	}

	VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerFactory::createInfo() {
		return createInfo_;
	}
}
