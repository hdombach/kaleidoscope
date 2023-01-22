#pragma once

#include "Instance.h"
#include "Window.h"
#include <memory>
namespace vulkan {
	class Surface;
	using SharedSurface = std::shared_ptr<Surface>;
	using UniqueSurface = std::unique_ptr<Surface>;

	class Surface {
		public:
			static SharedSurface createShared(vulkan::SharedInstance instance, vulkan::SharedWindow window);
			static UniqueSurface createUnique(vulkan::SharedInstance instance, vulkan::SharedWindow window);
			VkSurfaceKHR& operator*();
			~Surface();

		private:
			Surface(vulkan::SharedInstance, vulkan::SharedWindow);

			VkSurfaceKHR surface_;
			vulkan::SharedInstance instance_;
			vulkan::SharedWindow window_;
	};
}
