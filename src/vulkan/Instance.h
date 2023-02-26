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

	struct InstanceData {
		VkInstance instance_;
	};
	struct InstanceDeleter {
		void operator()(VkInstance *instance) const;
	};

	class Instance: public std::unique_ptr<VkInstance, InstanceDeleter> {
		public:
			Instance() = default;

			Instance(const char *name);

			VkInstance& raw();

			std::vector<const char*> *requiredExtensions();

		private:
			std::vector<const char*> requiredExtensions_;
	};
}

