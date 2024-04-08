#include <memory>

#include <GLFW/glfw3.h>

#include "App.hpp"
#include "./vulkan/graphics.hpp"
#include "./util/log.hpp"
#include "./types/ResourceManager.hpp"
#include "./vulkan/staticMesh.hpp"
#include "./vulkan/staticTexture.hpp"
#include "./vulkan/uiRenderPipeline.hpp"
#include "./ui/window.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/textureMaterial.hpp"
#include "vulkan/ColorMaterial.hpp"

App::App(std::string const &name) {
	vulkan::Graphics::initDefault("Kaleidoscope");
	_ui_render_pipeline = std::make_unique<vulkan::UIRenderPipeline>();
	_resource_manager = std::make_unique<types::ResourceManager>();

	if (auto viking_room = vulkan::StaticTexture::from_file("assets/viking_room.png")) {
		_resource_manager->add_texture("viking_room", viking_room.value());
	} else {
		util::log_error("Could not load example texture viking_room.png");
	}
	if (auto viking_room = vulkan::StaticMesh::fromFile("assets/viking_room.obj")) {
		_resource_manager->add_mesh("viking_room", viking_room.value());
	} else {
		util::log_error(util::f(viking_room.error()));
	}

	if (auto scene = vulkan::Scene::create(*_resource_manager)) {
		_scene = std::unique_ptr<vulkan::Scene>(new vulkan::Scene(std::move(scene.value())));
	} else {
		util::log_error(util::f(scene.error()));
	}

	{
		auto new_node = vulkan::Node(
				*_resource_manager->get_mesh("viking_room"),
				new vulkan::TextureMaterial(_resource_manager->get_texture("viking_room")));
		new_node.set_position({0, 0, 0});
		_scene->add_node(std::move(new_node));
	}

	{
		auto new_node = vulkan::Node(
				*_resource_manager->get_mesh("viking_room"),
				new vulkan::ColorMaterial({0.8, 0.2, 0.2}));
		new_node.set_position({0, 2, 0});
		_scene->add_node(std::move(new_node));
	}


	_window = std::unique_ptr<ui::Window>(new ui::Window(*_scene));
}

App::~App() {
	_resource_manager.reset();
	_scene.reset();
	_window.reset();
	_ui_render_pipeline.reset();
	vulkan::Graphics::deleteDefault();
	glfwTerminate();
}

void App::main_loop() {
	while (!glfwWindowShouldClose(vulkan::Graphics::DEFAULT->window())) {
		glfwPollEvents();

		_scene->render_preview();
		_ui_render_pipeline->submit([&] {
			_window->show();	
		});
	}

	vulkan::Graphics::DEFAULT->waitIdle();
}
