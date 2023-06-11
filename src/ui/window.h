#pragma once
#include "graphics.h"
#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>


namespace ui {
	class Window {
		public:
			static void show(VkImageView viewport, vulkan::Graphics const &graphics);
	};
}
