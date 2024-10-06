#pragma once

#include "../vulkan/SceneTexture.hpp"

namespace vulkan {
	class Scene;
}

namespace ui {
	struct State {
		enum SceneTab {
			Nothing,
			Camera,
			Nodes,
			Textures,
			Meshes,
			Materials,
		};

		static std::unique_ptr<State> create(vulkan::Scene &scene) {
			return std::unique_ptr<State>(new State{vulkan::SceneTexture(0, scene)});
		}
		uint32_t selected_node() {
			if (scene_tab == Nodes) {
				return selected_item;
			}
			return 0;
		}

		vulkan::SceneTexture scene_texture;
		bool showing_preview = true;
		glm::vec2 prev_mouse_pos = glm::vec2(0);
		int selected_item = -1;
		SceneTab scene_tab = Nodes;
		std::string selected_name = "";
		bool dup_name_error;
		uint32_t selected_shader_resource;
	};
}
