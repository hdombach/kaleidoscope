#pragma once

#include <vector>
#include <vulkan/vulkan.h>
namespace vulkan {

#ifdef NDEBUG
	static bool ENABLE_VALIDATION_LAYERS = false;
#else
	static bool ENABLE_VALIDATION_LAYERS = true;
#endif

	static std::vector<const char*> VALIDATION_LAYERS = {
			"VK_LAYER_KHRONOS_validation"
	};

	static std::vector<const char*> DEVICE_EXTENSIONS = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
}
