#include "textureView.hpp"
#include "../util/format.hpp"
#include "../util/log.hpp"
#include "../vulkan/texture.hpp"
#include <glm/fwd.hpp>
#include <imgui.h>

namespace ui {
	TextureView::TextureView(vulkan::Texture &texture): texture_(texture) {}

	void TextureView::show() {
		auto widgetSize = ImGui::GetContentRegionAvail();
		if (widgetSize.x >= 0 && widgetSize.y >= 0) {
			if (texture_.isResizable() && (widgetSize.x != size_.x || widgetSize.y != size_.y)) {
				size_ = widgetSize;
				texture_.resize(glm::ivec2{size_.x, size_.y});
			}
		}

		ImGui::Image(texture_.getDescriptorSet(), widgetSize);
	}
}
