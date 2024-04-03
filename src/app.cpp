#include <memory>

#include <GLFW/glfw3.h>

#include "app.hpp"
#include "./vulkan/descriptorPool.hpp"
#include "./vulkan/graphics.hpp"
#include "./util/log.hpp"
#include "./types/resourceManager.hpp"
#include "./vulkan/staticMesh.hpp"
#include "./vulkan/staticTexture.hpp"
#include "./vulkan/uiRenderPipeline.hpp"
#include "./ui/window.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/textureMaterial.hpp"

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

	resourceManager_->addMaterial("viking_room", new vulkan::TextureMaterial(resourceManager_->getTexture("viking_room")));

	if (auto scene_res = vulkan::Scene::create(*resourceManager_)) {
		_scene = std::unique_ptr<vulkan::Scene>(new vulkan::Scene(std::move(scene_res.value())));
	} else {
		util::log_error(util::f(scene_res.error()));
	}

	_scene->add_node(vulkan::Node(*resourceManager_->getMesh("viking_room"), *resourceManager_->getMaterial("viking_room")));

	window_ = std::unique_ptr<ui::Window>(new ui::Window(*_scene));
}

App::~App() {
	resourceManager_.reset();
	_scene.reset();
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
