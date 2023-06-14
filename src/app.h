#pragma once

#include "uiRenderPipeline.h"
#include "window.h"
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <memory>
#include "vulkan/graphics.h"


class App {
	public:
		App(std::string const &name);
		~App();
		void mainLoop();

	private:
		std::unique_ptr<vulkan::Graphics> graphics_;
		std::unique_ptr<vulkan::UIRenderPipeline> uiRenderPipeline_;
		std::unique_ptr<ui::Window> window_;
};
