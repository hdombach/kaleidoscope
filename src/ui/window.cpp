#include "window.h"
#include "defs.h"
#include "format.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "log.h"
#include "mainRenderPipeline.h"
#include "resourceManager.h"
#include "textureView.h"
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
