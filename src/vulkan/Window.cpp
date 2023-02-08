#include "Window.h"
#include <iostream>

namespace vulkan {
	SharedWindow Window::createShared(WindowCreateInfo &createInfo) {
		return SharedWindow(new Window(createInfo));
	}

	UniqueWindow Window::createUnique(WindowCreateInfo &createInfo) {
		return UniqueWindow(new Window(createInfo));
	}

	GLFWwindow* Window::operator*() {
		return window_;
	}

	GLFWwindow* Window::raw() {
		return window_;
	}

	Window::~Window() {
		glfwDestroyWindow(window_);
	}

	Window::Window(WindowCreateInfo &createInfo) {
		std::cout << "name is " << createInfo.name << std::endl;
		window_ = glfwCreateWindow(createInfo.width, createInfo.height, createInfo.name, nullptr, nullptr);
	}

	WindowFactory &WindowFactory::defaultSetup() {
		createInfo.width = 800;
		createInfo.height = 600;
		createInfo.name = "Kaleidoscope";

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		return *this;
	}

	SharedWindow WindowFactory::createShared() {
		return Window::createShared(createInfo);
	}

	UniqueWindow WindowFactory::createUnique() {
		return Window::createUnique(createInfo);
	}
}
