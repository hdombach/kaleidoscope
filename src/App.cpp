#include <memory>
#include <filesystem>

#include <GLFW/glfw3.h>

#include "App.hpp"
#include "./vulkan/graphics.hpp"
#include "./util/log.hpp"
#include "./types/ResourceManager.hpp"
#include "./vulkan/StaticTexture.hpp"
#include "./vulkan/UIRenderPipeline.hpp"
#include "./ui/AppView.hpp"
#include "vulkan/Scene.hpp"
#include "util/file.hpp"

float comb_ratio_value = 0.0f;

App::Ptr App::create(std::string const &name) {
	auto result = new App();
	vulkan::Graphics::init_default("Kaleidoscope");
	result->_ui_render_pipeline = std::make_unique<vulkan::UIRenderPipeline>();
	result->_resource_manager = std::make_unique<types::ResourceManager>();

	{
		auto res = result->_resource_manager->add_texture_from_file("assets/viking_room.png");
		if (!res) {
			LOG_ERROR << "Could not load example texture viking_room.png: " << res.value() << std::endl;
		}
	}

	{
		auto res = result->_resource_manager->add_texture_from_file("assets/grunge.png");
		if (!res) {
			LOG_ERROR << "Could not load example texture \"grunge.png\": " << res.value() << std::endl;
		}
	}

	{
		auto res = result->_resource_manager->add_mesh_from_file("assets/viking_room.obj");
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}
	}

	{
		auto res = result->_resource_manager->add_mesh_square("square");
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}
	}

	{
		auto res = result->_resource_manager->add_texture_material(
				"viking_room",
				result->_resource_manager->get_texture("viking_room"));
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}
	}

	{
		auto res = result->_resource_manager->add_texture_material(
				"grunge",
				result->_resource_manager->get_texture("grunge"));
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}
	}

	if (auto res = result->_resource_manager->add_comb_texture_material(
				"grunge_comb", 
				result->_resource_manager->get_texture("viking_room"), 
				result->_resource_manager->get_texture("grunge")))
	{ } else {
		LOG_ERROR << res.error().desc() << std::endl;
	}

	{
		auto res = result->_resource_manager->add_color_material("color", glm::vec3(0.5, 0.1, 0.2));
		if (!res) {
			LOG_ERROR << res.error().desc() << std::endl;
		}
	}

	if (auto scene = vulkan::Scene::create(*(result->_resource_manager))) {
		result->_scene = std::move(scene.value());
	} else {
		LOG_ERROR << scene.error().desc() << std::endl;
	}

	{
		auto id = result->_scene->add_node(
				result->_resource_manager->get_mesh("viking_room"),
				result->_resource_manager->get_material("grunge_comb"));
		result->_scene->get_node_mut(id.value())->set_position({0, 2.5, 0});
		result->_scene->get_node_mut(id.value())->resources().add_resource(types::ShaderResource::create_primitive("comb_ratio", comb_ratio_value));
	}
	{
		auto id = result->_scene->add_node(
				result->_resource_manager->get_mesh("square"),
				result->_resource_manager->get_material("color"));
		result->_scene->get_node_mut(id.value())->set_position({0, 3.0, 0});
		result->_scene->get_node_mut(id.value())->resources().add_resource(types::ShaderResource::create_color("color", glm::vec3(0.2, 0.3, 1.0)));
	}

	result->_view_state = ui::State::create(*result->_scene);
	result->_prev_semaphore = nullptr;

	return std::unique_ptr<App>(result);
}

App::~App() {
	_scene.reset();
	_resource_manager.reset();
	_view_state.reset();
	_ui_render_pipeline.reset();
	vulkan::Graphics::delete_default();
	glfwTerminate();
}

void App::main_loop() {
	while (!glfwWindowShouldClose(vulkan::Graphics::DEFAULT->window())) {
		glfwPollEvents();

		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
		comb_ratio_value = 0.5 + 0.4 * sin(time * 1);

		if (!(*_scene->begin()).get()) {
			LOG_DEBUG << "bool: " << (_scene->begin() == _scene->end()) << std::endl;
			LOG_DEBUG << "empty" << std::endl;
		}

		for (auto &node : *_scene) {
			if (auto comb_ratio = node->resources().get("comb_ratio")) {
				//LOG_DEBUG << "comb_ratio: " << comb_ratio.value().as_float() << std::endl;
				//comb_ratio.value().set_float(comb_ratio_value);
			}
		}

		VkSemaphore semaphore;
		_scene->update();
		_scene->set_selected_node(_view_state->selected_node());
		if (_view_state->showing_preview) {
			semaphore = _scene->render_preview(_prev_semaphore);
		} else {
			semaphore = _scene->render_raytrace(_prev_semaphore);
		}
		_prev_semaphore = _ui_render_pipeline->submit([&] {
				ui::AppView(*this, *_view_state);
		}, semaphore);
		//TODO: temp
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
