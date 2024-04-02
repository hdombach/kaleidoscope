#include "app.hpp"
#include "./vulkan/descriptorPool.hpp"
#include "./vulkan/graphics.hpp"
#include "./util/log.hpp"
#include "./vulkan/materialFactory.hpp"
#include "./types/resourceManager.hpp"
#include "./vulkan/staticMesh.hpp"
#include "./vulkan/staticTexture.hpp"
#include "./vulkan/uiRenderPipeline.hpp"
#include "./ui/window.hpp"
#include <memory>
#include <GLFW/glfw3.h>
#include <stdexcept>

App::App(std::string const &name) {
	vulkan::Graphics::initDefault("Kaleidoscope");
	descriptorPool_ = std::make_unique<vulkan::DescriptorPool>();
	uiRenderPipeline_ = std::make_unique<vulkan::UIRenderPipeline>();
	resourceManager_ = std::make_unique<types::ResourceManager>();
	if (auto vikingRoom = vulkan::StaticTexture::from_file("assets/viking_room.png")) {
		resourceManager_->addTexture("viking_room", vikingRoom.value());
	} else {
		util::log_error("Could not load example texture viking_room.png");
	}
	if (auto vikingRoom = vulkan::StaticMesh::fromFile("assets/viking_room.obj")) {
		resourceManager_->addMesh("viking_room", vikingRoom.value());
	} else {
		util::log_error(util::f(vikingRoom.error()));
	}
	auto window = ui::Window::create(*resourceManager_);
	if (!window.has_value()) {
		throw std::runtime_error("Error when creating window: " + window.error().desc());
	}
	window_ = std::move(window.value());
	materialFactory_ = std::make_unique<vulkan::MaterialFactory>(window_->main_render_pipeline(), *descriptorPool_);
	resourceManager_->addMaterial("viking_room", materialFactory_->textureMaterial(resourceManager_->getTexture("viking_room")));
	resourceManager_->getMaterial("viking_room")->add_preview(window_->main_render_pipeline());
}

App::~App() {
	resourceManager_.reset();
	window_.reset();
	uiRenderPipeline_.reset();
	descriptorPool_.reset();
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
