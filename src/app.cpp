#include "app.h"
#include "graphics.h"
#include "log.h"
#include "resourceManager.h"
#include "staticTexture.h"
#include "uiRenderPipeline.h"
#include "window.h"
#include <memory>
#include <GLFW/glfw3.h>

App::App(std::string const &name) {
	vulkan::Graphics::initDefault("Kaleidoscope");
	uiRenderPipeline_ = std::make_unique<vulkan::UIRenderPipeline>();
	resourceManager_ = std::make_unique<types::ResourceManager>();
	if (auto vikingRoom = vulkan::StaticTexture::fromFile("assets/viking_room.png")) {
		resourceManager_->addTexture("viking_room", vikingRoom.value());
	} else {
		util::log_error("Could not load example texture viking_room.png");
	}
	window_ = std::make_unique<ui::Window>(*resourceManager_);
}

App::~App() {
	window_.reset();
	resourceManager_.reset();
	uiRenderPipeline_.reset();
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
