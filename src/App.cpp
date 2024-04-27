#include <memory>
#include <filesystem>

#include <GLFW/glfw3.h>

#include "App.hpp"
#include "./vulkan/graphics.hpp"
#include "./util/log.hpp"
#include "./types/ResourceManager.hpp"
#include "./vulkan/StaticMesh.hpp"
#include "./vulkan/StaticTexture.hpp"
#include "./vulkan/UIRenderPipeline.hpp"
#include "./ui/window.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/TextureMaterial.hpp"
#include "vulkan/ColorMaterial.hpp"
#include "util/file.hpp"

App::App(std::string const &name) {
	vulkan::Graphics::init_default("Kaleidoscope");
	_ui_render_pipeline = std::make_unique<vulkan::UIRenderPipeline>();
	_resource_manager = std::make_unique<types::ResourceManager>();

	if (auto viking_room = vulkan::StaticTexture::from_file(util::env_file_path("assets/viking_room.png"))) {
		_resource_manager->add_texture("viking_room", viking_room.value());
	} else {
		LOG_ERROR << "Could not load example texture viking_room.png" << std::endl;
	}
	if (auto viking_room = vulkan::StaticMesh::from_file("assets/viking_room.obj")) {
		_resource_manager->add_mesh("viking_room", viking_room.value());
	} else {
		LOG_ERROR << viking_room.error().desc() << std::endl;
	}

	if (auto scene = vulkan::Scene::create(*_resource_manager)) {
		_scene = std::unique_ptr<vulkan::Scene>(new vulkan::Scene(std::move(scene.value())));
	} else {
		LOG_ERROR << scene.error().desc() << std::endl;
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
	vulkan::Graphics::delete_default();
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

	vulkan::Graphics::DEFAULT->wait_idle();
}

std::string &App::prog_path() {
	return _prog_path;
}

std::string &App::working_path() {
	return _working_path;
}

void App::set_prog_path(std::string path) {
	_prog_path = path;
	_working_path = std::filesystem::path(_prog_path).parent_path();
}

std::string App::_prog_path;
std::string App::_working_path;
