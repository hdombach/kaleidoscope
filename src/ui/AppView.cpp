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
		auto scene_size = ImVec2(app.scene().camera().width, app.scene().camera().height);
		int render_rate = app.scene().render_rate();

		ImGui::Begin("Viewport");

		ImGui::Text("offsett: %f, %f", mouse_offset.x, mouse_offset.y);
		TextureView(state.scene_texture, scene_size);
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
		ImGui::DragInt("Render rate", &render_rate, 200);
		ImGui::Separator();
		CameraView::show(app.scene().camera());
		SceneView(app.scene(), state);
		ImGui::End();

		app.scene().set_is_preview(state.showing_preview);
		app.scene().set_render_rate(render_rate);
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
				state.scene_tab = State::Nodes;
				NodesView(scene, state);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Textures")) {
				state.scene_tab = State::Textures;
				ImGui::Text("hello");
				TexturesEditView(scene.resource_manager(), state);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::Begin("Selected");
		switch (state.scene_tab) {
			case State::Nodes:
				NodeView(scene, scene.get_node_mut(state.selected_item), state);
				break;
			case State::Textures:
				TextureEditView(scene.resource_manager(), scene.resource_manager().get_texture(state.selected_item), state);
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
				if (state.selected_item == node->id()) {
					state.selected_item = -1;
				} else {
					state.selected_item = node->id();
					state.selected_name = node->name();
				}
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
			ShaderResourcesView(node->resources(), scene.resource_manager(), state);
		} else {
			ImGui::Text("No node selected");
		}

	}

	void TexturesEditView(types::ResourceManager &resources, State &state) {
		float width = 250;

		ImGui::BeginChild("Texture List", ImVec2(width, -ImGui::GetFrameHeightWithSpacing()), true);
		for (auto texture : util::Adapt(resources.texture_begin(), resources.texture_end())) {
			if (!texture) continue;
			if (ImGui::Selectable(texture->name().data(), state.selected_item == texture->id())) {
				if (state.selected_item == texture->id()) {
					state.selected_item = -1;
				} else {
					state.selected_item = texture->id();
					state.selected_name = texture->name();
					state.dup_name_error = false;
				}
			}
		}
		ImGui::EndChild();
		if (ImGui::Button("Add texture", ImVec2(width, 0))) {
			auto urls = pfd::open_file("Select an image", ".", {"Image Files"}).result();
			resources.add_texture_from_file(urls[0]);
		}
	}

	void TextureEditView(
			types::ResourceManager &resources,
			vulkan::Texture *texture,
			State &state)
	{
		if (texture) {
			if (ui::InputText("##SelectedName", &state.selected_name, ImGuiInputTextFlags_EnterReturnsTrue)) {
				state.dup_name_error = !resources.rename_texture(texture->id(), state.selected_name);
			}
			ImGui::SameLine();
			if (ImGui::Button("Set Name")) {
				resources.rename_texture(texture->id(), state.selected_name);
			}
			if (state.dup_name_error) {
				ImGui::TextColored({1.0, 0.0, 0.0, 1.0}, "ERROR: Duplicate name");
			}
			ImGui::Image(texture->imgui_descriptor_set(), ImVec2(250, 250));
		} else {
			ImGui::Text("No texture selected");
		}
	}

	void ShaderResourcesView(
			types::ShaderResources &shader_resources,
			types::ResourceManager &resources,
			State &state)
	{
		float width = 250;
		ImGui::Text("Shader Resources");
		if (ImGui::BeginTable("Shader Resource", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			for (auto r : shader_resources.get()) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%s:", r->name().data());
				ImGui::TableNextColumn();
				switch (r->type()) {
					case types::ShaderResource::Type::Texture:
						SelectTextureView(
								*r, 
								shader_resources,
								resources,
								state);
						break;
					case types::ShaderResource::Type::Float:
						ShaderResourceFloatView(*r, shader_resources, state);
						break;
					case types::ShaderResource::Type::Vec3:
						ShaderResourceVec3View(*r, shader_resources, state);
						break;
					case types::ShaderResource::Type::Color3:
						ShaderResourceColorView(*r, shader_resources, state);
						break;
					default:
						ImGui::Text("Unknown");
						break;
				}
			}
			ImGui::EndTable();
		}
	}

	void ShaderResourceFloatView(
			const types::ShaderResource &resource,
			types::ShaderResources &resources,
			State &state)
	{
		float value = resource.as_float().value();
		std::string name = "##shader_resource_" + resource.name();
		if (ImGui::DragFloat(name.data(), &value, 0.01)) {
			resources.set_float(resource.name(), value);
		}
	}

	void ShaderResourceVec3View(
			const types::ShaderResource &resource,
			types::ShaderResources &resources,
			State &state)
	{
		auto v = util::as_array(resource.as_vec3().value());
		std::string name = "##shader_resource_" + resource.name();
		ImGui::DragFloat3(name.data(), v.data());
		resources.set_vec3(resource.name(), util::as_vec(v));
	}

	void ShaderResourceColorView(
			const types::ShaderResource &resource,
			types::ShaderResources &resources,
			State &state)
	{
		auto color = util::as_array(resource.as_color3().value());
		std::string name = "##shader_resource_" + resource.name();
		ImGui::ColorPicker3(name.data(), color.data());
		resources.set_color3(resource.name(), util::as_vec(color));
	}

	void SelectTextureView(
			types::ShaderResource const &resource,
			types::ShaderResources &resources,
			types::ResourceManager &resource_manager,
			State &state)
	{
		std::string name = "##shader_resource_" + resource.name();
		if (resource.type() != types::ShaderResource::Type::Texture) {
			return;
		}
		auto &texture = resource.as_texture().value();
		if (ImGui::BeginCombo(name.data(), texture.name().data())) {
			for (auto t : util::Adapt(resource_manager.texture_begin(), resource_manager.texture_end())) {
				if (!t) continue;
				if (ImGui::Selectable(t->name().data(), texture.id() == t->id())) {
					if (texture.id() != t->id()) {
						resources.set_texture(resource.name(), t);
					}
				}
			}
			ImGui::EndCombo();
		}
	}
}
