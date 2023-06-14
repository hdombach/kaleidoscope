#pragma once

#include "texture.h"
namespace ui {
	class TextureView {
		public:
			TextureView(vulkan::Texture const &texture);
			void show() const;

		private:
			vulkan::Texture const &texture_;
	};
}
