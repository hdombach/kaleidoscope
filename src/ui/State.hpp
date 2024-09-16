#pragma once

#include "../vulkan/SceneTexture.hpp"

namespace vulkan {
	class Scene;
}

namespace ui {
	struct State {
		enum SceneTab {
			Nothing,
			Nodes,
			Textures,
		};

		static std::unique_ptr<State> create(vulkan::Scene &scene) {
			return std::unique_ptr<State>(new State{vulkan::SceneTexture(0, scene)});
		}
		vulkan::SceneTexture scene_texture;
		bool showing_preview = true;
		glm::vec2 prev_mouse_pos = glm::vec2(0);
		int selected_item = 0;
		SceneTab _scene_tab = Nodes;
	};
}
