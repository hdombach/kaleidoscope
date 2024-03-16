#pragma once
#include <memory>
#include <vulkan/vulkan.h>

#include "../vulkan/mainRenderPipeline.hpp"
#include "../types/resourceManager.hpp"
#include "textureView.hpp"


namespace ui {
	class Window {
		public:
			using Ptr = std::unique_ptr<Window>;

			static util::Result<Ptr, KError> create(types::ResourceManager &resource_manager);

			~Window();

			void show();
			vulkan::MainRenderPipeline const &main_render_pipeline() const;
		private:
			Window(vulkan::MainRenderPipeline::Ptr &&main_render_pipeline);

			std::unique_ptr<vulkan::MainRenderPipeline> _main_render_pipeline;
			TextureView _viewport;
	};
}
