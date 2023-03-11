#include "Window.h"
#include "Device.h"
#include "ImageView.h"
#include "RenderPass.h"
#include "Swapchain.h"
#include "Graphics.h"
#include <iostream>

namespace vulkan {
	Window::Window(const char *name, uint32_t width, uint32_t height) {
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		auto window = new GLFWwindow*;
		*window = glfwCreateWindow(width, height, name, nullptr, nullptr);
		this->reset(window);
		glfwSetFramebufferSizeCallback(*window, framebufferResizeCallback_);

	}

	GLFWwindow* Window::raw() {
		return **this;
	}

	void Window::set_graphics(Graphics *graphics) {
		this->graphics_ = graphics;
		glfwSetWindowUserPointer(raw(), this);
	}

	void Window::framebufferResizeCallback_(GLFWwindow *window, int width, int height) {
		auto self = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		if (self->graphics_) {
			self->graphics_->triggerResize();
		} else {
			throw std::runtime_error("Graphics was no provided to window class");
		}
	}

	void WindowDeleter::operator()(GLFWwindow **window) {
		glfwDestroyWindow(*window);
		delete window;
	}
}
