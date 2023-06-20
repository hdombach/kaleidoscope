#include "app.h"
#include "graphics.h"
#include "log.h"
#include "materialFactory.h"
#include "resourceManager.h"
#include "staticMesh.h"
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
	if (auto vikingRoom = vulkan::StaticMesh::fromFile("assets/viking_room.obj")) {
		resourceManager_->addMesh("viking_room", vikingRoom.value());
	} else {
		util::log_error(util::f(vikingRoom.error()));
	}
	window_ = std::make_unique<ui::Window>(*resourceManager_);
	materialFactory_ = std::make_unique<vulkan::MaterialFactory>(window_->mainRendePipeline());
	resourceManager_->addMaterial("viking_room", materialFactory_->textureMaterial(resourceManager_->getTexture("viking_room")));
}

App::~App() {
	resourceManager_.reset();
	window_.reset();
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
