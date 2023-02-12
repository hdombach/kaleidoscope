#include "Window.h"
#include "Device.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include <iostream>

namespace vulkan {
	Window::Window(const char *name, uint32_t width, uint32_t height) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		auto window = new GLFWwindow*;
		*window = glfwCreateWindow(width, height, name, nullptr, nullptr);

		this->reset(window);
	}

	GLFWwindow* Window::raw() {
		return **this;
	}

	void WindowDeleter::operator()(GLFWwindow **window) {
		glfwDestroyWindow(*window);
		delete window;
	}
}
