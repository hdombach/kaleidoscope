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

	void ShaderResourceFloatView(
			types::ShaderResource const &resource,
			types::ShaderResources &resources,
			State &state);

	void ShaderResourceVec3View(
			types::ShaderResource const &resource,
			types::ShaderResources &resources,
			State &state);

	void ShaderResourceColorView(
			types::ShaderResource const &resource,
			types::ShaderResources &resources,
			State &state);

	void SelectTextureView(
			types::ShaderResource const &resource,
			types::ShaderResources &resources,
			types::ResourceManager &resources_manager,
			State &state);
}
