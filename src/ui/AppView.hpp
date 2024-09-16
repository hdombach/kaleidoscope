#pragma once

#include <glm/fwd.hpp>

#include "textureView.hpp"
#include "../vulkan/SceneTexture.hpp"
#include "State.hpp"
#include "../types/Node.hpp"
#include "../types/ResourceManager.hpp"

class App;
namespace ui {
	void AppView(App &app, State &state);
	glm::vec3 get_cam_movement();

	void SceneView(vulkan::Scene &scene, State &state);
	void NodesView(vulkan::Scene &scene, State &state);
	void NodeView(vulkan::Scene &scene, vulkan::Node *node, State &state);

	void TexturesView(types::ResourceManager &resources, State &state);
	void TextureView(
			types::ResourceManager &resources,
			vulkan::Texture *texture,
			State &state);
}
