#pragma once
#include "graphics.h"
#include "mainRenderPipeline.h"
#include "textureView.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vulkan/vulkan.h>


namespace ui {
	class Window {
		public:
			Window(vulkan::Graphics const &graphics);
			~Window();
			void show();
		private:
			vulkan::Graphics const &graphics_;
			std::unique_ptr<vulkan::MainRenderPipeline> mainRenderPipeline_;
			TextureView viewport_;

			void tempLoadModel_();
	};
}
