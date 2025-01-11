#pragma once

#include <memory>
#include <string>

#include <vulkan/vulkan.h>

#include "vulkan/UIRenderPipeline.hpp"
#include "vulkan/Scene.hpp"
#include "types/ResourceManager.hpp"
#include "ui/State.hpp"


class App {
	public:
		using Ptr = std::unique_ptr<App>;
		static Ptr create(std::string const &name);
		~App();
		void main_loop();
		vulkan::Scene &scene() { return *_scene; };

	private:
		App() = default;
		vulkan::Scene::Ptr _scene;
		std::unique_ptr<vulkan::UIRenderPipeline> _ui_render_pipeline;
		std::unique_ptr<types::ResourceManager> _resource_manager;
		std::unique_ptr<ui::State> _view_state;
		VkSemaphore _prev_semaphore;
};
