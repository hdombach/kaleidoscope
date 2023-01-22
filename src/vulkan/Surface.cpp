#include "Surface.h"
#include "Error.h"
#include "Instance.h"
#include "Window.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	SharedSurface Surface::createShared(vulkan::SharedInstance instance, vulkan::SharedWindow window) {
		return SharedSurface(new Surface(instance, window));
	}

	UniqueSurface Surface::createUnique(vulkan::SharedInstance instance, vulkan::SharedWindow window) {
		return UniqueSurface(new Surface(instance, window));
	}

	VkSurfaceKHR& Surface::operator*() {
		return surface_;
	}

	Surface::~Surface() {
		vkDestroySurfaceKHR(**instance_, surface_, nullptr);
	}

	Surface::Surface(vulkan::SharedInstance instance, vulkan::SharedWindow window): instance_(instance), window_(window) {
		auto result = glfwCreateWindowSurface(**instance_, **window_, nullptr, &surface_);
		if (result != VK_SUCCESS) {
			throw Error(result);
		}
	}
}
