#pragma once

#include <glm/fwd.hpp>

#include "textureView.hpp"
#include "../vulkan/SceneTexture.hpp"
#include "State.hpp"
#include "../types/Node.hpp"
#include "../types/ResourceManager.hpp"
#include "../types/ShaderResource.hpp"

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

	void ShaderResourcesView(
			types::ShaderResources &shader_resources,
			types::ResourceManager &resources,
			State &state);
	void ShaderResourceView(
			types::ShaderResource &shader_resource,
			types::ResourceManager &resources,
			State &state);

	void SelectTextureView(types::ResourceManager &resources, uint32_t &selected);
}
