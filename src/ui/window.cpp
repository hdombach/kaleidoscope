#include "window.hpp"
#include "defs.hpp"
#include "format.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "log.hpp"
#include "mainRenderPipeline.hpp"
#include "resourceManager.hpp"
#include "textureView.hpp"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <vector>

namespace ui {
	Window::Window(types::ResourceManager &resourceManager):
		mainRenderPipeline_(std::make_unique<vulkan::MainRenderPipeline>(resourceManager, VkExtent2D{300, 300})),
		viewport_(*mainRenderPipeline_)
	{}

	Window::~Window() {
		mainRenderPipeline_.reset();
	}

	void Window::show() {
		mainRenderPipeline_->submit();
		auto imguiViewport = ImGui::GetMainViewport();
		//ImGui::SetNextWindowPos(imguiViewport->WorkPos);
		//ImGui::SetNextWindowSize(imguiViewport->WorkSize);

		ImGui::Begin("Hello World Example");
		ImGui::Text("Hello World");

		viewport_.show();

		ImGui::End();
	}

	vulkan::MainRenderPipeline const &Window::mainRendePipeline() const {
		return *mainRenderPipeline_;
	}
}
