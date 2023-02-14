#pragma once

#include "Instance.h"
#include "Window.h"
#include "vulkan/vulkan_core.h"
#include <memory>
namespace vulkan {
	class Surface;
	using SharedSurface = std::shared_ptr<Surface>;

	struct SurfaceData {
		VkSurfaceKHR surface_;
		SharedInstance instance_;
		SharedWindow window_;
	};
	struct SurfaceDeleter {
		void operator()(SurfaceData *data) const;
	};

	class Surface: public std::unique_ptr<SurfaceData, SurfaceDeleter> {
		public:
			using base_type = std::unique_ptr<SurfaceData, SurfaceDeleter>;

			Surface(SharedInstance instance, SharedWindow window);
			VkSurfaceKHR& raw();
	};
}
