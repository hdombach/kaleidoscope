#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <string>
namespace vulkan {

#ifdef NDEBUG
	static bool ENABLE_VALIDATION_LAYERS = false;
#else
	static bool ENABLE_VALIDATION_LAYERS = true;
#endif

	static const uint32_t WIDTH = 800;
	static const uint32_t HEIGHT = 600;
	static const int MAX_FRAMES_IN_FLIGHT = 2;
	static const std::string MODEL_PATH = "assets/viking_room.obj";
	static const std::string TEXTURE_PATH = "assets/viking_room.png";

	static std::vector<const char*> VALIDATION_LAYERS = {
			"VK_LAYER_KHRONOS_validation"
	};

	static std::vector<const char*> DEVICE_EXTENSIONS = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
}
