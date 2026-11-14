#pragma once

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.hpp>

#include "util/result.hpp"
#include "Error.hpp"
#include "Fence.hpp"
#include "Semaphore.hpp"

namespace vulkan {
	struct SwapchainDeleter {
		VkDevice device;

		void operator()(VkSwapchainKHR swapchain) {
			vkDestroySwapchainKHR(device, swapchain, nullptr);
		}
	};
	using SwapchainPtr = std::unique_ptr<VkSwapchainKHR_T, SwapchainDeleter>;



	class Window {
		public:
			static constexpr uint32_t IMAGE_COUNT = 2;
			static constexpr uint32_t SEMAPHORE_COUNT = IMAGE_COUNT + 1;

			struct Frame {
				VkCommandBuffer command_buffer = nullptr;
				Fence fence;
				VkImage image = nullptr;
				VkImageView image_view = nullptr;
				VkFramebuffer framebuffer = nullptr;
			};

			using Ptr = std::unique_ptr<Window>;

		public:
			Window() = default;

			util::Result<Ptr, Error> create(
				int width,
				int height
			);

			Frame *begin_frame();
			void end_frame();

		private:
			util::Result<VkSurfaceFormatKHR, Error> _select_surface_format();
			util::Result<VkPresentModeKHR, Error> _select_present_mode();

			util::Result<void, Error> _recreate_window();
		private:
			int _width = 0, _height = 0;
			uint32_t _frame_index = 0;
			bool _swapchain_rebuild = false;
			VkSwapchainKHR _swapchain = nullptr;
			VkPhysicalDevice _physical_device = nullptr;
			VkDevice _device = nullptr;
			VkSurfaceKHR _surface = nullptr;
			VkRenderPass _render_pass = nullptr;
			std::array<Frame, IMAGE_COUNT> _frames;
			std::array<Semaphore, SEMAPHORE_COUNT> _semaphores;
	};
}
