#include <imgui.h>
#include <memory>
#include <string>
#include <chrono>

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "App.hpp"
#include "vulkan/Scene.hpp"
#include "vulkan/graphics.hpp"
#include "vulkan/UIRenderPipeline.hpp"
#include "util/log.hpp"
#include "types/ResourceManager.hpp"
#include "ui/AppView.hpp"
#include "util/log.hpp"

float comb_ratio_value = 0.0f;

App::Ptr App::create(std::string const &name) {
	uint32_t r;
	auto result = new App();
	vulkan::Graphics::init_default("Kaleidoscope");
	result->_ui_render_pipeline = std::make_unique<vulkan::UIRenderPipeline>();
	result->_resource_manager = std::make_unique<types::ResourceManager>();

	if (auto err = result->_resource_manager->add_texture_from_file("assets/viking_room.png").move_or(r)) {
		log_error(err.value()) << "Could not add viking room texture" << std::endl;
	}
	if (auto err = result->_resource_manager->add_texture_from_file("assets/grunge.png").move_or(r)) {
		log_error(err.value()) << "Could not add grunge texture" << std::endl;
	}

	if (auto err = result->_resource_manager->add_mesh_from_file("assets/viking_room.obj").move_or(r)) {
		log_error(err.value()) << "Could not add viking room mesh" << std::endl;
	}
	if (auto err = result->_resource_manager->add_mesh_square("square").move_or(r)) {
		log_error(err.value()) << "Could not add square mesh" << std::endl;
	}
	if (auto err = result->_resource_manager->add_mesh_mandelbulb("mandelbulb").move_or(r)) {
		log_error(err.value()) << "Could not add mandelbulb mesh" << std::endl;
	}
	if (auto err = result->_resource_manager->add_mesh_mandelbox("mandelbox").move_or(r)) {
		log_error(err.value()) << "Could not add mandelbox mesh" << std::endl;
	}

	if (auto err = result->_resource_manager->add_texture_material(
			"viking_room",
			result->_resource_manager->get_texture("viking_room")).move_or(r)
	) {
		log_error(err.value()) << "Could not add viking room texture material" << std::endl;
	}
	if (auto err = result->_resource_manager->add_texture_material(
			"grunge",
			result->_resource_manager->get_texture("grunge")).move_or(r)
	) {
		log_error(err.value()) << "Could not add grunge texture material" << std::endl;
	}
	if (auto err = result->_resource_manager->add_comb_texture_material(
				"grunge_comb", 
				result->_resource_manager->get_texture("viking_room"), 
				result->_resource_manager->get_texture("grunge")).move_or(r)) {
		log_error(err.value()) << "Could not add grungy viking room material" << std::endl;
	}
	if (auto err = result->_resource_manager->add_color_material(
			"color",
			glm::vec3(0.5, 0.1, 0.2)).move_or(r)
	) {
		log_error(err.value()) << "Could not add color material" << std::endl;
	}

	if (auto scene = vulkan::Scene::create(*(result->_resource_manager))) {
		result->_scene = std::move(scene.value());
	} else {
		log_fatal_error() << scene.error() << std::endl;
	}

	if (auto node = result->_scene->create_node(
				result->_resource_manager->get_mesh("viking_room"),
				result->_resource_manager->get_material("grunge_comb")))
	{
		node.value()->set_position({0, 2.5, 0});
		node.value()->resources().add_resource(
			types::ShaderResource::create_primitive("comb_ratio", comb_ratio_value)
		);
	} else {
		log_error() << node.error() << std::endl;
	}

	if (auto node = result->_scene->create_node(
				result->_resource_manager->get_mesh("square"),
				result->_resource_manager->get_material("color")))
	{
		node.value()->set_position({0, 3, 0});
		node.value()->resources().add_resource(
			types::ShaderResource::create_color("color", glm::vec3(0.2, 0.3, 1.0))
		);
	} else {
		log_error() << node.error() << std::endl;
	}

	{
		{
			auto id = result->_scene->create_node(
				result->_resource_manager->get_mesh("mandelbulb"), 
				result->_resource_manager->get_material("color")
			);
		}

		{
			auto id = result->_scene->create_virtual_node();
		}
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

		VkSemaphore semaphore;
		_scene->update();
		_scene->set_selected_node(_view_state->selected_node());
		if (_view_state->showing_preview) {
			semaphore = _scene->render_preview(_prev_semaphore);
		} else {
			semaphore = _scene->render_raytrace(_prev_semaphore);
		}
		_prev_semaphore = _ui_render_pipeline->submit([&] {
			//ImGui::ShowDemoWindow();
			ui::AppView(*this, *_view_state);
		}, semaphore);
		//TODO: temp
	}

	vulkan::Graphics::DEFAULT->wait_idle();
}
