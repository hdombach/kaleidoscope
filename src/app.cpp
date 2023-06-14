#include "app.h"
#include "graphics.h"
#include "uiRenderPipeline.h"
#include "window.h"
#include <memory>
#include <GLFW/glfw3.h>

App::App(std::string const &name) {
	vulkan::Graphics::initDefault("Kaleidoscope");
	uiRenderPipeline_ = std::make_unique<vulkan::UIRenderPipeline>(*vulkan::Graphics::DEFAULT);
	window_ = std::make_unique<ui::Window>(*vulkan::Graphics::DEFAULT);
}

App::~App() {
	uiRenderPipeline_.reset();
	window_.reset();
	vulkan::Graphics::deleteDefault();
	glfwTerminate();
}

void App::mainLoop() {
	while (!glfwWindowShouldClose(vulkan::Graphics::DEFAULT->window())) {
		glfwPollEvents();

		uiRenderPipeline_->submit([&] {
			window_->show();	
		});
	}

	vulkan::Graphics::DEFAULT->waitIdle();
}
