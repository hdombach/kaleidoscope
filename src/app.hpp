#pragma once

#include "./types/ResourceManager.hpp"
#include "./vulkan/uiRenderPipeline.hpp"
#include "./ui/window.hpp"
#include "vulkan/Scene.hpp"

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <memory>


class App {
	public:
		App(std::string const &name);
		~App();
		void main_loop();

	private:
		std::unique_ptr<vulkan::Scene> _scene;
		std::unique_ptr<vulkan::UIRenderPipeline> _ui_render_pipeline;
		std::unique_ptr<ui::Window> _window;
		std::unique_ptr<types::ResourceManager> _resource_manager;
};
