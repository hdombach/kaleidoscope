#include "window.h"
#include "imgui.h"

namespace ui {
	void Window::show() {
		auto viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);

		ImGui::Begin("Hello World Example");
		ImGui::Text("Hello World");
		ImGui::End();
	}
}
