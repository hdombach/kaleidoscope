#include <glm/fwd.hpp>
#include <imgui.h>

#include "textureView.hpp"
#include "../vulkan/Texture.hpp"

namespace ui {
	TextureView::TextureView(vulkan::Texture &texture): _texture(texture) {}

	void TextureView::show(vulkan::Texture &texture) {
		auto widget_size = ImGui::GetContentRegionAvail();
		if (widget_size.x >= 0 && widget_size.y >= 0 && texture.is_resizable()) {
			texture.resize(VkExtent2D{
					static_cast<uint32_t>(widget_size.x),
					static_cast<uint32_t>(widget_size.y)
			});
		}

		ImGui::ImageButton(
				texture.imgui_descriptor_set(),
				widget_size,
				ImVec2(0, 0),
				ImVec2(1, 1),
				0);
	}
}
