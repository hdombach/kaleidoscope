#include "Surface.h"
#include "Error.h"
#include "Instance.h"
#include "Window.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {

	void SurfaceDeleter::operator()(SurfaceData *data) const {
		vkDestroySurfaceKHR(data->instance_->raw(), data->surface_, nullptr);
		delete data;
	}

	Surface::Surface(SharedInstance instance, SharedWindow window):
		base_type(new SurfaceData{nullptr, instance, window})
	{
		auto result = glfwCreateWindowSurface(instance->raw(), window->raw(), nullptr, &raw());
		if (result != VK_SUCCESS) {
			throw Error(result);
		}
	}

	VkSurfaceKHR& Surface::raw() {
		return get()->surface_;
	}
}
