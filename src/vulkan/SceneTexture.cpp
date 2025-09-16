#include "SceneTexture.hpp"
#include "Scene.hpp"

namespace vulkan {
	SceneTexture::SceneTexture(uint32_t id, Scene &scene):
		_scene(scene),
		_id(id),
		_name("Internal Scene")
	{ }

	ImTextureID SceneTexture::imgui_id() {
		return reinterpret_cast<ImTextureID>(_scene.imgui_descriptor_set());
	}

	VkImageView SceneTexture::image_view() const {
		return _scene.image_view();
	}

	uint32_t SceneTexture::id() const {
		return _id;
	}

	void SceneTexture::resize(VkExtent2D size) {
		_scene.resize(size);
	}
}
