#pragma once

#include "./vulkan/materialFactory.hpp"
#include "./types/resourceManager.hpp"
#include "./vulkan/uiRenderPipeline.hpp"
#include "./vulkan/descriptorPool.hpp"
#include "./ui/window.hpp"
#define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>
#include <memory>
#include "vulkan/graphics.hpp"


class App {
	public:
		App(std::string const &name);
		~App();
		void mainLoop();

	private:
		std::unique_ptr<vulkan::UIRenderPipeline> uiRenderPipeline_;
		std::unique_ptr<ui::Window> window_;
		std::unique_ptr<types::ResourceManager> resourceManager_;
		std::unique_ptr<vulkan::MaterialFactory> materialFactory_;
		std::unique_ptr<vulkan::DescriptorPool> descriptorPool_;
};
