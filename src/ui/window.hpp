#pragma once
#include <memory>
#include <vulkan/vulkan.h>

#include "../vulkan/PreviewRenderPass.hpp"
#include "../types/resourceManager.hpp"
#include "textureView.hpp"


namespace ui {
	class Window {
		public:
			using Ptr = std::unique_ptr<Window>;

			static util::Result<Ptr, KError> create(types::ResourceManager &resource_manager);

			~Window();

			void show();
			vulkan::PreviewRenderPass &main_render_pipeline();
			vulkan::PreviewRenderPass const &main_render_pipeline() const;
		private:
			Window(vulkan::PreviewRenderPass::Ptr &&main_render_pipeline);

			std::unique_ptr<vulkan::PreviewRenderPass> _main_render_pipeline;
			TextureView _viewport;
	};
}
