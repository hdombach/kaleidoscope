#pragma once
#include <memory>
#include <vulkan/vulkan.h>

#include "../vulkan/Scene.hpp"
#include "textureView.hpp"


namespace ui {
	class Window {
		public:
			Window(vulkan::Scene &scene);
			using Ptr = std::unique_ptr<Window>;

			~Window() = default;

			void show();
		private:
			vulkan::Scene &_scene;

			TextureView _viewport;
	};
}
