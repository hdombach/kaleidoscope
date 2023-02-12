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
	};

	void DebugUtilsMessengerDeleter::operator()(DebugUtilsMessengerData *data) const {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(data->instance_->raw(), "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(data->instance_->raw(), data->messenger_, nullptr);
		}
		delete data;
	};

	DebugUtilsMessenger::DebugUtilsMessenger(SharedInstance instance):
		base_type(new DebugUtilsMessengerData{nullptr, instance})
	{
		auto createInfo = defaultConfig();
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance->raw(), "vkCreateDebugUtilsMessengerEXT");
		VkResult result;
		if (func != nullptr) {
			result = func(instance->raw(), &createInfo, nullptr, &get()->messenger_);
		} else {
			result = VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessenger::defaultConfig() {
		auto createInfo = VkDebugUtilsMessengerCreateInfoEXT{};
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

		createInfo.pNext = nullptr;

		return createInfo;
	}

	VkDebugUtilsMessengerEXT& DebugUtilsMessenger::operator*() {
		return get()->messenger_;
	}

	VkDebugUtilsMessengerEXT* DebugUtilsMessenger::operator->() {
		return &get()->messenger_;
	}

	VkDebugUtilsMessengerEXT& DebugUtilsMessenger::raw() {
		return get()->messenger_;
	}
}
