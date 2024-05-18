#pragma once

#include "../vulkan/Texture.hpp"
#include <imgui.h>
namespace ui {
	class TextureView {
		public:
			TextureView(vulkan::Texture &texture);
			inline void show() { show(_texture); }
			static void show(vulkan::Texture &texture);

		private:
			vulkan::Texture &_texture;
			ImVec2 _size;
	};
}
