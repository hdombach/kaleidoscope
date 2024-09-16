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
		float width = 250;

		ImGui::Separator();
		ImGui::Text("Nodes");
		ImGui::BeginChild("Node List", ImVec2(width, -ImGui::GetFrameHeightWithSpacing()), true);
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
		if (ImGui::Button("New node", ImVec2(width, 0))) {
			if (auto id = scene.add_node(scene.resource_manager().default_mesh(), scene.resource_manager().default_material())) {
				state.selected_node = id.value();
			}
			LOG_DEBUG << "created new node: " << state.selected_node << std::endl;
		}

		ImGui::Begin("Node");
		if (scene.get_node_mut(state.selected_node)) {
			NodeView(scene, *scene.get_node_mut(state.selected_node), state);
		} else {
			ImGui::Text("No node selected");
		}
		ImGui::End();
	}

	void NodeView(vulkan::Scene &scene, vulkan::Node &node, State &state) {
		auto pos = util::as_array(node.position());

		ImGui::PushID(node.id());
		ImGui::Text("Node");
		ui::InputText("Name", &node.name());
		ImGui::DragFloat3("Position", pos.data(), 0.01f);
		if (ImGui::Button("Delete")) {
			scene.rem_node(node.id());
		}
		ImGui::PopID();

		node.set_position(util::as_vec(pos));
	}
}
