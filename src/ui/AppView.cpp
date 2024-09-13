#include <cstdio>
#include <glm/fwd.hpp>
#include <imgui.h>

#include "AppView.hpp"
#include "CameraView.hpp"
#include "../App.hpp"
#include "../util/Util.hpp"
#include "Misc.hpp"

namespace ui {
	void AppView(App &app, State &state) {
		auto mouse_raw = ImGui::GetMousePos();
		auto cur_mouse_pos = glm::vec2(mouse_raw.x, mouse_raw.y);
		auto mouse_offset = cur_mouse_pos - state.prev_mouse_pos;

		ImGui::Begin("Viewport");

		ImGui::Text("offsett: %f, %f", mouse_offset.x, mouse_offset.y);
		TextureView::show(state.scene_texture);
		if (ImGui::IsItemHovered()) {
			auto &camera = app.scene().camera();
			if (ImGui::IsMouseDown(0)) {
				camera.rotate_drag(mouse_offset * -0.004f);
			}
			camera.position += get_cam_movement() * camera.rotation * 0.01f;
		}
		ImGui::End();

		ImGui::Begin("Settings");
		ImGui::Checkbox("Showing preview", &state.showing_preview);
		ImGui::Separator();
		CameraView::show(app.scene().camera());
		SceneView(app.scene(), state);
		ImGui::End();

		app.scene().set_is_preview(state.showing_preview);
		state.prev_mouse_pos = cur_mouse_pos;
	}

	glm::vec3 get_cam_movement() {
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

	void SceneView(vulkan::Scene &scene, State &state) {
		char name_buf[128];
		char *name;

		ImGui::Separator();
		ImGui::Text("Nodes");
		ImGui::BeginChild("Node List", ImVec2(150, 0), true);
		for (auto &node : scene) {
			if (node->name().empty()) {
				snprintf(name_buf, sizeof(name_buf), "Node %d", node->id());
				name = name_buf;
			} else {
				name = node->name().data();
			}

			if (ImGui::Selectable(name, state.selected_node == node->id())) {
				state.selected_node = node->id();
			}
		}
		ImGui::EndChild();

		ImGui::Begin("Node");
		NodeView(*scene.get_node_mut(state.selected_node), state);
		ImGui::End();
	}

	void NodeView(vulkan::Node &node, State &state) {
		auto pos = util::as_array(node.position());

		ImGui::PushID(node.id());
		ImGui::Text("Node");
		ui::InputText("Name", &node.name());
		ImGui::DragFloat3("Position", pos.data(), 0.01f);
		ImGui::PopID();

		node.set_position(util::as_vec(pos));
	}
}
