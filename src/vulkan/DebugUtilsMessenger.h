#pragma once

#include "Instance.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vulkan/vulkan.h>

namespace vulkan {
	//callbacks
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
			void *pUserData);

	class DebugUtilsMessenger;
	using SharedDebugUtilsMessenger = std::shared_ptr<DebugUtilsMessenger>;

	struct DebugUtilsMessengerData {
		VkDebugUtilsMessengerEXT messenger_;
		SharedInstance instance_;
	};

	struct DebugUtilsMessengerDeleter {
		void operator()(DebugUtilsMessengerData *data) const;
	};

	class DebugUtilsMessenger: std::unique_ptr<DebugUtilsMessengerData, DebugUtilsMessengerDeleter> {
		public:
			using base_type = std::unique_ptr<DebugUtilsMessengerData, DebugUtilsMessengerDeleter>;

			DebugUtilsMessenger() = default;
			DebugUtilsMessenger(SharedInstance instance);

			static VkDebugUtilsMessengerCreateInfoEXT defaultConfig();

			VkDebugUtilsMessengerEXT& raw();
	};
}
