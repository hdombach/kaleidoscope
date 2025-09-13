#pragma once

#include <glm/fwd.hpp>

#include "State.hpp"
#include "types/Node.hpp"
#include "types/ResourceManager.hpp"
#include "types/ShaderResource.hpp"

class App;
namespace ui {
	void AppView(App &app, State &state);
	glm::vec4 get_cam_movement();

	void SceneView(vulkan::Scene &scene, State &state);
	void NodesView(vulkan::Scene &scene, State &state);
	void NodeItemView(vulkan::Scene &scene, vulkan::Node &node, State &state);
	void NodeSelectedView(vulkan::Scene &scene, vulkan::Node *node, State &state);

	void TextureListView(types::ResourceManager &resources, State &state);
	void TextureSelectedView(
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

	void MeshListView(
			types::ResourceManager &resources,
			State &state);

	void MeshSelectedView(
			types::ResourceManager &resources,
			types::Mesh *mesh,
			State &state);

	void MaterialListView(
			types::ResourceManager &resources,
			State &state);

	void MaterialSelectedView(
			types::ResourceManager &resources,
			types::Material *material,
			State &state);
}
