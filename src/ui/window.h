#pragma once
#include "mainRenderPipeline.h"
#include "textureView.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vulkan/vulkan.h>


namespace ui {
	class Window {
		public:
			Window();
			~Window();
			void show();
		private:
			std::unique_ptr<vulkan::MainRenderPipeline> mainRenderPipeline_;
			TextureView viewport_;

			void tempLoadModel_();
	};
}
