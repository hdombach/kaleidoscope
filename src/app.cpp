#include "app.h"
#include "graphics.h"
#include <memory>
#include <GLFW/glfw3.h>

App::App(std::string const &name) {
	graphics_ = std::make_unique<vulkan::Graphics>("Kaleidoscope");
}

App::~App() {
	graphics_.reset();
	glfwTerminate();
}

void App::mainLoop() {
	while (!glfwWindowShouldClose(graphics_->window())) {
		glfwPollEvents();
		graphics_->tick();
	}

	graphics_->waitIdle();
}
