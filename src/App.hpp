#pragma once

#include "./types/ResourceManager.hpp"
#include "./vulkan/UIRenderPipeline.hpp"
#include "./ui/AppView.hpp"
#include "vulkan/Scene.hpp"

#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <memory>


class App {
	public:
		using Ptr = std::unique_ptr<App>;
		static Ptr create(std::string const &name);
		~App();
		void main_loop();
		vulkan::Scene &scene() { return *_scene; };

		static std::string &prog_path();
		static std::string &working_path();
		static void set_prog_path(std::string path);

	private:
		App() = default;
		vulkan::Scene::Ptr _scene;
		std::unique_ptr<vulkan::UIRenderPipeline> _ui_render_pipeline;
		std::unique_ptr<types::ResourceManager> _resource_manager;
		std::unique_ptr<ui::AppView> _app_view;

		static std::string _prog_path;
		static std::string _working_path;
};
