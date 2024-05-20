#include "AppView.hpp"
#include "../App.hpp"
#include <glm/fwd.hpp>
#include <imgui.h>

namespace ui {
	AppView::AppView(App& app):
		_app(app),
		_scene_viewport(app.scene()),
		_showing_preview(true)
	{}

	void AppView::show() {
		auto mouse_raw = ImGui::GetMousePos();
		auto cur_mouse_pos = glm::vec2(mouse_raw.x, mouse_raw.y);
		auto mouse_offset = cur_mouse_pos - _previous_mouse_pos;

		ImGui::Begin("Viewport");

		ImGui::Text("offset: %f, %f", mouse_offset.x, mouse_offset.y);
		_scene_viewport.show();
		if (ImGui::IsItemHovered()) {
			auto &camera = _app.scene().camera();
			if (ImGui::IsMouseDown(0)) {
				camera.rotate_drag(mouse_offset * -0.004f);
			}
			camera.position += _get_camera_movement() * camera.rotation * 0.01f;
		}
		ImGui::End();

		ImGui::Begin("Settings");
		ImGui::Checkbox("Showing preview", &_showing_preview);
		ImGui::End();

		_app.scene().set_is_preview(_showing_preview);
		_previous_mouse_pos = cur_mouse_pos;
	}

	glm::vec3 AppView::_get_camera_movement() {
		auto result = glm::vec3(0);

		if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_W))) {
			result.z += 1;
		}
		if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_S))) {
			result.z -= 1;
		}

		if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_D))) {
			result.x -= 1;
		}
		if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A))) {
			result.x += 1;
		}

		if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_E))) {
			result.y += 1;
		}
		if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_Q))) {
			result.y -= 1;
		}

		return result;
	}
}
