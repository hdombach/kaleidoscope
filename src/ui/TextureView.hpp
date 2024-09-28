#pragma once

#include "../vulkan/Texture.hpp"
#include <imgui.h>
namespace ui {
	void TextureView(vulkan::Texture &texture);
	void TextureView(vulkan::Texture &texture, ImVec2 size);
}
