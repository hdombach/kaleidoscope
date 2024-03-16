#include <memory>

#include <stdexcept>
#include <tiny_obj_loader.h>
#include <vulkan/vulkan_core.h>
#include <imgui.h>

#include "window.hpp"
#include "../vulkan/mainRenderPipeline.hpp"
#include "../types/resourceManager.hpp"
#include "textureView.hpp"

namespace ui {
	util::Result<Window::Ptr, KError> Window::create(types::ResourceManager &resource_manager) {
		auto render_pipeline = vulkan::MainRenderPipeline::create(resource_manager, {300, 300});
		RETURN_IF_ERR(render_pipeline);
		return Window::Ptr(new Window(std::move(render_pipeline.value())));
	}
	Window::~Window() {
		_main_render_pipeline.reset();
	}

	void Window::show() {
		_main_render_pipeline->submit();
		auto imguiViewport = ImGui::GetMainViewport();
		//ImGui::SetNextWindowPos(imguiViewport->WorkPos);
		//ImGui::SetNextWindowSize(imguiViewport->WorkSize);

		ImGui::Begin("Hello World Example");
		ImGui::Text("Hello World");

		_viewport.show();

		ImGui::End();
	}

	vulkan::MainRenderPipeline const &Window::main_render_pipeline() const {
		return *_main_render_pipeline;
	}

	Window::Window(vulkan::MainRenderPipeline::Ptr &&main_render_pipeline):
		_main_render_pipeline(std::move(main_render_pipeline)),
		_viewport(*_main_render_pipeline)
	{}
}
