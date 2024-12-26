#pragma once

#include <functional>

#include <imgui.h>
#include <vulkan/vulkan.h>

#include "Semaphore.hpp"
#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	class Graphics;

	class UIRenderPipeline {
		public:
			UIRenderPipeline();
			~UIRenderPipeline();

			VkSemaphore submit(
					std::function<void()> ui_callback,
					VkSemaphore semaphore);

		private:
			void _create_descriptor_pool();
			void _init_im_gui();
			void _setup_vulkan_window(int width, int height);

			VkSemaphore _render_frame(ImDrawData *draw_data, VkSemaphore semaphore);
			void _present_frame();

		private:
			VkDescriptorPool _descriptor_pool;
			ImGui_ImplVulkanH_Window _window_data;
			ImGuiIO *_io;
			bool _swapchain_rebuild;
			constexpr static const ImVec4 _clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
			Semaphore _semaphore;
	};
}
