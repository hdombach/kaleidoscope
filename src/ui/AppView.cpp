#include <cstdio>
#include <string>

#include <glm/fwd.hpp>
#include <imgui.h>
#include <portable-file-dialogs.h>

#include "AppView.hpp"
#include "CameraView.hpp"
#include "App.hpp"
#include "Misc.hpp"
#include "State.hpp"
#include "ui/TextureView.hpp"
#include "util/KError.hpp"
#include "util/Util.hpp"
#include "types/ResourceManager.hpp"
#include "types/Material.hpp"
#include "types/Mesh.hpp"
#include "util/log.hpp"

namespace ui {
	void AppView(App &app, State &state) {
		auto mouse_raw = ImGui::GetMousePos();
		auto cur_mouse_pos = glm::vec2(mouse_raw.x, mouse_raw.y);
		auto mouse_offset = cur_mouse_pos - state.prev_mouse_pos;
		auto scene_size = ImVec2(
			app.scene().camera().width(),
			app.scene().camera().height()
		);
		int render_rate = app.scene().render_rate();

		ImGui::Begin("Viewport");

		ImGui::Text("offsett: %f, %f", mouse_offset.x, mouse_offset.y);
		TextureView(state.scene_texture, scene_size);
		if (ImGui::IsItemHovered()) {
			auto &camera = app.scene().camera();
			if (ImGui::IsMouseDown(0)) {
				camera.rotate_drag(mouse_offset * -0.004f);
			}
			auto p = get_cam_movement() * camera.gen_rotate_mat() * 0.01f;
			camera.set_position(camera.position() + glm::vec3(p.x, p.y, p.z));
		}
		ImGui::End();

		SceneView(app.scene(), state);

		state.prev_mouse_pos = cur_mouse_pos;
	}

	glm::vec4 get_cam_movement() {
		auto result = glm::vec4(0);

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
		int render_rate = scene.render_rate();
		ImGui::Begin("Scene");
		ImGui::Checkbox("Showing preview", &state.showing_preview);
		ImGui::DragInt("Render rate", &render_rate, 200);
		scene.set_is_preview(state.showing_preview);
		scene.set_render_rate(render_rate);

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
				TextureListView(scene.resource_manager(), state);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Meshes")) {
				state.scene_tab = State::Meshes;
				MeshListView(scene.resource_manager(), state);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Materials")) {
				state.scene_tab = State::Materials;
				MaterialListView(scene.resource_manager(), state);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::Begin("Selected");
		switch (state.scene_tab) {
			case State::Nodes:
				NodeSelectedView(scene, scene.get_node_mut(state.selected_item), state);
				break;
			case State::Textures:
				TextureSelectedView(scene.resource_manager(), scene.resource_manager().get_texture(state.selected_item), state);
				break;
			case State::Meshes:
				MeshSelectedView(scene.resource_manager(), scene.resource_manager().get_mesh(state.selected_item), state);
				break;
			case State::Materials:
				MaterialSelectedView(scene.resource_manager(), scene.resource_manager().get_material(state.selected_item), state);
				break;
			default:
				ImGui::Text("Nothing selected");
				break;
		}
		ImGui::End(); // Selected
		ImGui::End(); // Scene
	}

	void NodesView(vulkan::Scene &scene, State &state) {
		char name_buf[128];
		char *name;
		//float width = 250;
		ImGui::BeginChild("Node List", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
		float width = (ImGui::GetWindowSize().x - ImGui::GetFrameHeightWithSpacing()) / 2;
		NodeItemView(scene, *scene.root(), state);
		ImGui::EndChild();

		log_debug() << (state.selected_item == 0) << ", " << state.selected_item << std::endl;
		ImGui::BeginDisabled(state.selected_item == State::SELECTED_NONE || state.selected_item == scene.root()->id());
		if (ImGui::Button("Delete node", ImVec2(width, 0))) {
			if (state.selected_item != 0) {
				scene.remove_node(state.selected_item);
			}
		}
		ImGui::EndDisabled();

		ImGui::SameLine(0, 0);
		if (ImGui::Button("New node", ImVec2(width, 0))) {
			if (auto node = scene.create_node(scene.resource_manager().default_mesh(), scene.resource_manager().default_material())) {
				state.selected_item = node.value()->id();
				log_event() << "Created node " << state.selected_item << std::endl;
			} else {
				log_error() << node.error() << std::endl;
			}
		}
		ImGui::SameLine(0, 0);
		if (ImGui::BeginCombo("##Create combo", "", ImGuiComboFlags_NoPreview | ImGuiComboFlags_PopupAlignLeft)) {
			if (ImGui::Selectable("New virtual node")) {
				if (auto n = scene.create_virtual_node()) {
					state.selected_item = n.value()->id();
					log_event() << "Created virtual node " << state.selected_item << std::endl;
				} else {
					log_error() << n.error() << std::endl;
				}
			}
			if (ImGui::Selectable("New camera")) {
				if (auto n = scene.create_camera()) {
					state.selected_item = n.value()->id();
					log_event() << "Created camera " << state.selected_item << std::endl;
				} else {
					log_error() << n.error() << std::endl;
				}
			}
			ImGui::EndCombo();
		}
	}

	void NodeItemView(vulkan::Scene &scene, vulkan::Node &node, State &state) {
		char name_buf[128];
		char *name;
		ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_OpenOnArrow;

		if (node.name().empty()) {
			snprintf(name_buf, sizeof(name_buf), "Node %d", node.id());
			name = name_buf;
		} else {
			name = node.name().data();
		}

		if (state.selected_item == node.id()) {
			tree_flags |= ImGuiTreeNodeFlags_Selected;
		}


		bool show_children = ImGui::TreeNodeEx(node.name().data(), tree_flags);
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			if (state.selected_item == node.id()) {
				state.selected_item = State::SELECTED_NONE;
			} else {
				state.selected_item = node.id();
				state.selected_name = node.name();
			}
		}
		if (node.id() != scene.root()->id() && ImGui::BeginDragDropSource()) {
			ImGui::SetDragDropPayload("NodeItems", &node, sizeof(vulkan::Node));
			ImGui::Text("%s", node.name().data());
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget()) {
			if (auto payload = ImGui::AcceptDragDropPayload("NodeItems")) {
				auto n = static_cast<vulkan::Node const *>(payload->Data);
				scene.get_node_mut(n->id())->move_to(&node);
			}
		}
		if (show_children) {
			for (auto &child : node) {
				NodeItemView(scene, *child, state);
			}
			ImGui::TreePop();
		}
	}

	void NodeSelectedView(vulkan::Scene &scene, vulkan::Node *node, State &state) {
		if (!node) {
			ImGui::Text("No node selected");
			return;
		}

		auto pos = util::as_array(node->position());
		auto rotation = util::as_array(node->rotation());
		auto scale = util::as_array(node->scale());
		ImGui::PushID(node->id());
		ImGui::Text("Node (id: %d)", node->id());
		ui::InputText("Name", &node->name());
		if (node->type() == vulkan::Node::Type::Object) {
			if (ImGui::BeginCombo("Mesh", node->mesh().name().data())) {
				for (auto &mesh : scene.resource_manager().meshes()) {
					if (!mesh) continue;
					if (ImGui::Selectable(mesh->name().data(), mesh->id() == node->mesh().id())) {
						node->set_mesh(*mesh);
					}
				}
				ImGui::EndCombo();
			}

			if (ImGui::BeginCombo("Material", node->material().name().data())) {
				for (auto &material : scene.resource_manager().materials()) {
					if (!material) continue;
					if (ImGui::Selectable(material->name().data(), material->id() == node->material().id())) {
						node->set_material(*material);
					}
				}
				ImGui::EndCombo();
			}
		}

		ImGui::DragFloat3("Position", pos.data(), 0.01f);
		ImGui::DragFloat3("Rotation", rotation.data(), 0.01f);
		ImGui::DragFloat3("Scale", scale.data(), 0.01f);
		ImGui::BeginDisabled(node->id() == scene.root()->id());
		if (ImGui::Button("Delete")) {
			scene.remove_node(node->id());
		}
		ImGui::EndDisabled();
		ImGui::PopID();
		node->set_position(util::as_vec(pos));
		node->set_rotation(util::as_vec(rotation));
		node->set_scale(util::as_vec(scale));
		ShaderResourcesView(node->resources(), scene.resource_manager(), state);
	}

	void TextureListView(types::ResourceManager &resources, State &state) {
		float width = 250;

		ImGui::BeginChild("Texture List", ImVec2(width, -ImGui::GetFrameHeightWithSpacing()), true);
		for (auto &texture : resources.textures()) {
			if (ImGui::Selectable(texture->name().data(), state.selected_item == texture->id())) {
				if (state.selected_item == texture->id()) {
					state.selected_item = 0;
				} else {
					state.selected_item = texture->id();
					state.selected_name = texture->name();
					state.dup_name_error = false;
				}
			}
		}
		ImGui::EndChild();
		if (ImGui::Button("Add texture", ImVec2(width, 0))) {
			auto urls = pfd::open_file("Select an image", ".", {"Image Files", "*.png *.jpg *.jpeg *.bmp"}).result();
			resources.add_texture_from_file(urls[0]);
		}
	}

	void TextureSelectedView(
			types::ResourceManager &resources,
			vulkan::Texture *texture,
			State &state)
	{
		if (texture) {
			ImGui::Text("Texture (id: %d)", texture->id());
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
		if (ImGui::BeginCombo(name.data(), texture->name().data())) {
			for (auto &t : resource_manager.textures()) {
				if (ImGui::Selectable(t->name().data(), texture->id() == t->id())) {
					if (texture->id() != t->id()) {
						log_trace() << "Setting texture " << t->id() << " of " << resource.name() << std::endl;
						resources.set_texture(resource.name(), t.get());
					}
				}
			}
			ImGui::EndCombo();
		}
	}

	void MeshListView(
			types::ResourceManager &resources,
			State &state)
	{
		float width = 250;

		ImGui::BeginChild("Mesh List", ImVec2(width, -ImGui::GetFrameHeightWithSpacing()), true);
		for (auto &mesh : resources.meshes()) {
			if (!mesh) continue;
			if (ImGui::Selectable(mesh->name().data(), state.selected_item == mesh->id())) {
				if (state.selected_item == mesh->id()) {
					state.selected_item = 0;
				} else {
					state.selected_item = mesh->id();
					state.selected_name = mesh->name();
					state.dup_name_error = false;
				}
			}
		}
		ImGui::EndChild();
		if (ImGui::Button("Add Object", ImVec2(width, 0))) {
			auto urls = pfd::open_file("Select a mesh", ".", {"Object file", "*.obj"}).result();
			resources.add_mesh_from_file(urls[0]);
		}
	}

	void MeshSelectedView(
			types::ResourceManager &resources,
			types::Mesh *mesh,
			State &state)
	{
		if (mesh) {
			ImGui::Text("Mesh (id: %d)", mesh->id());
			if (ui::InputText(
						"##SelectedName",
						&state.selected_name,
						ImGuiInputTextFlags_EnterReturnsTrue))
			{
				state.dup_name_error = !resources.rename_mesh(mesh->id(), state.selected_name);
			}
			ImGui::SameLine();
			if (ImGui::Button("Set Name")) {
				resources.rename_mesh(mesh->id(), state.selected_name);
			}
			if (state.dup_name_error) {
				ImGui::TextColored({1.0, 0.0, 0.0, 1.0}, "ERROR: Duplicate name");
			}
			ImGui::Text("No mesh preview");
		} else {
			ImGui::Text("No mesh selected");
		}
	}

	void MaterialListView(
			types::ResourceManager &resources,
			State &state)
	{
		float width = 250;

		ImGui::BeginChild("Material List", ImVec2(width, -ImGui::GetFrameHeightWithSpacing()), true);
		for (auto &material : resources.materials()) {
			if (!material) continue;
			if (ImGui::Selectable(material->name().data(), state.selected_item == material->id())) {
				if (state.selected_item == material->id()) {
					state.selected_item = 0;
				} else {
					state.selected_item = material->id();
					state.selected_name = material->name();
					state.dup_name_error = false;
				}
			}
		}
		ImGui::EndChild();
	}

	void MaterialSelectedView(
			types::ResourceManager &resources,
			types::Material *material,
			State &state)
	{
		if (material) {
			ImGui::Text("Material (id: %d)", material->id());
			if (ui::InputText("##SelectedName", &state.selected_name, ImGuiInputTextFlags_EnterReturnsTrue)) {
				state.dup_name_error = !resources.rename_material(material->id(), state.selected_name);
			}
			ImGui::SameLine();
			if (ImGui::Button("Set Name")) {
				resources.rename_material(material->id(), state.selected_name);
			}
			if (state.dup_name_error) {
				ImGui::TextColored({1.0, 0.0, 0.0, 1.0}, "ERROR: Duplicate name");
			}
			ImGui::Text("No preview right now");
		} else {
			ImGui::Text("No material selected");
		}
	}
}
