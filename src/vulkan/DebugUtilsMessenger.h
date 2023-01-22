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
	using UniqueDebugUtilsMessenger = std::unique_ptr<DebugUtilsMessenger>;

	class DebugUtilsMessenger {
		public:
			static SharedDebugUtilsMessenger create_shared(VkDebugUtilsMessengerCreateInfoEXT &createInfo, SharedInstance instance);
			static UniqueDebugUtilsMessenger create_unique(VkDebugUtilsMessengerCreateInfoEXT &createInfo, SharedInstance instance);
			VkDebugUtilsMessengerEXT& operator*();
			~DebugUtilsMessenger();

		private:
			DebugUtilsMessenger(VkDebugUtilsMessengerCreateInfoEXT &createInfo, SharedInstance instance);

			VkDebugUtilsMessengerEXT debugUtilsMessenger_;
			SharedInstance instance_;
	};

	class DebugUtilsMessengerFactory {
		public:
			DebugUtilsMessengerFactory();
			DebugUtilsMessengerFactory(SharedInstance instance);

			DebugUtilsMessengerFactory &default_config();
			SharedDebugUtilsMessenger create_shared();
			UniqueDebugUtilsMessenger create_unique();
			VkDebugUtilsMessengerCreateInfoEXT createInfo();

		private:
			VkDebugUtilsMessengerCreateInfoEXT createInfo_{};
			SharedInstance instance_;
	};
}
