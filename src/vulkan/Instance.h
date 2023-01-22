#pragma once

#include "Defs.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace vulkan {
	class Instance;
	using SharedInstance = std::shared_ptr<Instance>;
	using UniqueInstance = std::unique_ptr<Instance>;

	class Instance {
		public:
			static SharedInstance createShared(VkInstanceCreateInfo &createInfo);
			static UniqueInstance createUnique(VkInstanceCreateInfo &createInfo);
			VkInstance& operator*();
			~Instance();

		private:
			Instance(VkInstanceCreateInfo &createInfo);

			VkInstance instance;
	};

	class InstanceFactory {
		public:
			InstanceFactory();

			InstanceFactory &defaultConfig();
			SharedInstance createShared();
			UniqueInstance createUnique();

		private:
			void loadRequiredExtensions();
			bool checkValidationLayerSupport();

			VkInstanceCreateInfo createInfo{};
			VkApplicationInfo appInfo{};
			VkDebugUtilsMessengerCreateInfoEXT debugMessageCreateInfo{};
			std::vector<const char*> requiredExtensions;
	};
}

