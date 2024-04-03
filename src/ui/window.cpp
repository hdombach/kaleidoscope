#include <memory>

#include <stdexcept>
#include <tiny_obj_loader.h>
#include <vulkan/vulkan_core.h>
#include <imgui.h>

#include "window.hpp"
#include "../vulkan/PreviewRenderPass.hpp"
#include "../types/resourceManager.hpp"
#include "textureView.hpp"

namespace ui {
	Window::Window(vulkan::Scene &scene):
		_scene(scene),
		_viewport(scene.preview_texture())
	{}

	void Window::show() {
		auto imguiViewport = ImGui::GetMainViewport();
		//ImGui::SetNextWindowPos(imguiViewport->WorkPos);
		//ImGui::SetNextWindowSize(imguiViewport->WorkSize);

		ImGui::Begin("Hello World Example");
		ImGui::Text("Hello World");

		_viewport.show();

		ImGui::End();
	}
}
