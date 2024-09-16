#pragma once

#include <glm/fwd.hpp>

#include "textureView.hpp"
#include "../vulkan/SceneTexture.hpp"
#include "State.hpp"
#include "../types/Node.hpp"

class App;
namespace ui {
	void AppView(App &app, State &state);
	glm::vec3 get_cam_movement();

	void SceneView(vulkan::Scene &scene, State &state);
	void NodeView(vulkan::Scene &scene, vulkan::Node &node, State &state);
}
