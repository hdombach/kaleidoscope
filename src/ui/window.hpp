#pragma once
#include "mainRenderPipeline.hpp"
#include "resourceManager.hpp"
#include "textureView.hpp"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <vulkan/vulkan.h>


namespace ui {
	class Window {
		public:
			Window(types::ResourceManager &resourceManager);
			~Window();
			void show();
			vulkan::MainRenderPipeline const &mainRendePipeline() const;
		private:
			std::unique_ptr<vulkan::MainRenderPipeline> mainRenderPipeline_;
			TextureView viewport_;
	};
}
