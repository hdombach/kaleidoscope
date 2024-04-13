#pragma once

#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan_core.h"
#include <functional>
#include <imgui.h>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Graphics;

	class UIRenderPipeline {
		public:
			UIRenderPipeline();
			~UIRenderPipeline();

			void submit(std::function<void()> ui_callback);

		private:
			void _create_descriptor_pool();
			void _init_im_gui();
			void _setup_vulkan_window(int width, int height);

			void _render_frame(ImDrawData *draw_data);
			void _present_frame();

			VkDescriptorPool _descriptor_pool;
			ImGui_ImplVulkanH_Window _window_data;
			ImGuiIO *_io;
			bool _swapchain_rebuild;
			constexpr static const ImVec4 _clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	};
}
