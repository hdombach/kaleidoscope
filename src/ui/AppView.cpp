#include <cstdio>
#include <glm/fwd.hpp>
#include <imgui.h>
#include <portable-file-dialogs.h>
#include <filesystem>

#include "AppView.hpp"
#include "CameraView.hpp"
#include "../App.hpp"
#include "../util/Util.hpp"
#include "../util/IterAdapter.hpp"
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
		ImGui::Separator();
		if (ImGui::BeginTabBar("##SceneTabs")) {
			if (ImGui::BeginTabItem("Nodes")) {
				state._scene_tab = State::Nodes;
				NodesView(scene, state);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Textures")) {
				state._scene_tab = State::Textures;
				ImGui::Text("hello");
				TexturesView(scene.resource_manager(), state);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::Begin("Selected");
		switch (state._scene_tab) {
			case State::Nodes:
				NodeView(scene, scene.get_node_mut(state.selected_item), state);
				break;
			case State::Textures:
				TextureView(scene.resource_manager(), scene.resource_manager().get_texture(state.selected_item), state);
				break;
			default:
				ImGui::Text("Nothing selected");
				break;
		}
		ImGui::End();
	}

	void NodesView(vulkan::Scene &scene, State &state) {
		char name_buf[128];
		char *name;
		float width = 250;
		ImGui::BeginChild("Node List", ImVec2(width, -ImGui::GetFrameHeightWithSpacing()), true);
		for (auto &node : scene) {
			if (node->name().empty()) {
				snprintf(name_buf, sizeof(name_buf), "Node %d", node->id());
				name = name_buf;
			} else {
				name = node->name().data();
			}

			if (ImGui::Selectable(name, state.selected_item == node->id())) {
				state.selected_item = node->id();
			}
		}
		ImGui::EndChild();
		if (ImGui::Button("New node", ImVec2(width, 0))) {
			if (auto id = scene.add_node(scene.resource_manager().default_mesh(), scene.resource_manager().default_material())) {
				state.selected_item = id.value();
			}
			LOG_DEBUG << "created new node: " << state.selected_item << std::endl;
		}
	}

	void NodeView(vulkan::Scene &scene, vulkan::Node *node, State &state) {
		if (node) {
			auto pos = util::as_array(node->position());
			ImGui::PushID(node->id());
			ImGui::Text("Node");
			ui::InputText("Name", &node->name());
			ImGui::DragFloat3("Position", pos.data(), 0.01f);
			if (ImGui::Button("Delete")) {
				scene.rem_node(node->id());
			}
			ImGui::PopID();
			node->set_position(util::as_vec(pos));
		} else {
			ImGui::Text("No node selected");
		}

	}

	void TexturesView(types::ResourceManager &resources, State &state) {
		float width = 250;

		ImGui::BeginChild("Texture List", ImVec2(width, -ImGui::GetFrameHeightWithSpacing()), true);
		for (auto &texture : util::Adapt(resources.texture_begin(), resources.texture_end())) {
			if (ImGui::Selectable(texture->name().data(), state.selected_item == texture->id())) {
				state.selected_item = texture->id();
			}
		}
		ImGui::EndChild();
		if (ImGui::Button("Add texture", ImVec2(width, 0))) {
			auto urls = pfd::open_file("Select an image", ".", {"Image Files"}).result();
			auto name = std::filesystem::path(urls[0]).filename();
			resources.add_texture_from_file(name, urls[0]);
		}
	}

	void TextureView(
			types::ResourceManager &resources,
			vulkan::Texture *texture,
			State &state)
	{
		if (texture) {
			ImGui::Text("%s", texture->name().data());
			ImGui::Image(texture->imgui_descriptor_set(), ImVec2(250, 250));
		} else {
			ImGui::Text("No texture selected");
		}
	}
}
