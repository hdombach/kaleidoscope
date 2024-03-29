#pragma once

#include "../vulkan/texture.hpp"
#include <imgui.h>
namespace ui {
	class TextureView {
		public:
			TextureView(vulkan::Texture &texture);
			void show();

		private:
			vulkan::Texture &texture_;
			ImVec2 size_;
	};
}
