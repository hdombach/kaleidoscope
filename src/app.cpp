#include "app.h"
#include "graphics.h"
#include "uiRenderPipeline.h"
#include "window.h"
#include <memory>
#include <GLFW/glfw3.h>

App::App(std::string const &name) {
	graphics_ = std::make_unique<vulkan::Graphics>("Kaleidoscope");
	uiRenderPipeline_ = std::make_unique<vulkan::UIRenderPipeline>(*graphics_);
	window_ = std::make_unique<ui::Window>(*graphics_);
}

App::~App() {
	uiRenderPipeline_.reset();
	window_.reset();
	graphics_.reset();
	glfwTerminate();
}

void App::mainLoop() {
	while (!glfwWindowShouldClose(graphics_->window())) {
		glfwPollEvents();

		uiRenderPipeline_->submit([&] {
			window_->show();	
		});

		graphics_->drawFrame();
	}

	graphics_->waitIdle();
}
