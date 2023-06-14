#include "textureView.h"
#include "texture.h"
#include <imgui.h>

namespace ui {
	TextureView::TextureView(vulkan::Texture const &texture): texture_(texture) {}

	void TextureView::show() const {
		auto widgetSize = ImGui::GetContentRegionAvail();
		ImGui::Image(texture_.getDescriptorSet(), widgetSize);
	}
}
