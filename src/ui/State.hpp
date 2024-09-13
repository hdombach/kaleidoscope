#pragma once

#include "../vulkan/SceneTexture.hpp"

namespace vulkan {
	class Scene;
}

namespace ui {
	struct State {
		static std::unique_ptr<State> create(vulkan::Scene &scene) {
			return std::unique_ptr<State>(new State{
				vulkan::SceneTexture(0, scene),
				true,
				glm::vec2(0),
				0,
			});
		}
		vulkan::SceneTexture scene_texture;
		bool showing_preview;
		glm::vec2 prev_mouse_pos;
		uint32_t selected_node;
	};
}
