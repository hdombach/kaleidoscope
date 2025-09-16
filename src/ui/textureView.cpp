#include <glm/fwd.hpp>
#include <imgui.h>

#include "TextureView.hpp"
#include "vulkan/Texture.hpp"

namespace ui {
	void TextureView(vulkan::Texture &texture) {
		TextureView(texture, ImGui::GetContentRegionAvail());
	}
	void TextureView(vulkan::Texture &texture, ImVec2 size) {
		auto available = ImGui::GetContentRegionAvail();

		auto aspect =
			static_cast<float>(size.x)
			/ static_cast<float>(size.y);

		auto available_aspect =
			static_cast<float>(available.x)
			/ static_cast<float>(available.y);

		float rate;
		if (aspect > available_aspect) {
			rate = static_cast<float>(available.x) / static_cast<float>(size.x);
		} else {
			rate = static_cast<float>(available.y) / static_cast<float>(size.y);
		}
		size.x *= rate;
		size.y *= rate;

		auto pos = ImGui::GetCursorPos();
		pos.x += (available.x - size.x) / 2;
		pos.y += (available.y - size.y) / 2;

		ImGui::SetCursorPos(pos);

		ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, 0);
		ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 0);
		ImGui::ImageButton(
			texture.name().c_str(),
			texture.imgui_id(),
			size,
			ImVec2(0, 0),
			ImVec2(1, 1)
		);
		ImGui::PopStyleVar(2);

	}
}
