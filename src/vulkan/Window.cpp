#include "Window.hpp"

#include "vulkan/graphics.hpp"
#include "vulkan/vulkan_core.h"
#include <imgui_impl_vulkan.h>

namespace vulkan {
	util::Result<Window::Ptr, Error> Window::create(int width, int height) {
		auto ptr = std::make_unique<Window>();

		ptr->_width = width;
		ptr->_height = height;

		ptr->_device = Graphics::DEFAULT->device();
		ptr->_physical_device = Graphics::DEFAULT->physical_device();
		ptr->_surface = Graphics::DEFAULT->surface();

		return ptr;
	}

	Window::Frame *Window::begin_frame() {
		glfwPollEvents();

		if (_swapchain_rebuild) {
			int width, height;
			glfwGetFramebufferSize(Graphics::DEFAULT->window(), &width, &height);

			if (width > 0 && height > 0) {
				ImGui_ImplVulkan_SetMinImageCount(IMAGE_COUNT);
				if (auto err = _recreate_window().move_or()) {
					log_error(*err);
				}
				_frame_index = 0;
				_swapchain_rebuild = false;
			}
		}

		auto err = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, nullptr, nullptr, &_frame_index);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err ==  VK_SUBOPTIMAL_KHR) {
			_swapchain_rebuild = true;
			return nullptr;
		}

		auto &frame = _frames[_frame_index];

		if (auto err = frame.fence.wait()) {
			log_error(VkError(err));
		}
		frame.fence.reset();

		{
			auto begin_info = VkCommandBufferBeginInfo{};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			if (auto err = vkBeginCommandBuffer(frame.command_buffer, &begin_info)) {
				log_error(VkError(err));
				return nullptr;
			}
		}

		return &frame;
	}

	void Window::end_frame() {
		auto &frame = _frames[_frame_index];
		if (auto err = vkEndCommandBuffer(frame.command_buffer)) {
			log_error(VkError(err));
		}

		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = nullptr;
		submit_info.pWaitDstStageMask = nullptr;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &frame.command_buffer;
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = nullptr;

		if (auto err = vkQueueSubmit(
				Graphics::DEFAULT->queue(),
				1,
				&submit_info,
				*frame.fence
		)) {
			log_error(VkError(err));
		}
	}

	util::Result<VkSurfaceFormatKHR, Error> Window::_select_surface_format() {
		auto image_formats = std::array{
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_B8G8R8A8_UNORM
		};

		auto color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		uint32_t avail_count = 0;
		if (auto err = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &avail_count, nullptr)) {
			return Error(ErrorType::VULKAN, "Could not query surface formats", VkError(err));
		}
		auto formats = std::vector<VkSurfaceFormatKHR>(avail_count);
		if (auto err = vkGetPhysicalDeviceSurfaceFormatsKHR(_physical_device, _surface, &avail_count, formats.data())) {
			return Error(ErrorType::VULKAN, "Could not query surface formats", VkError(err));
		}

		for (auto &image_format : image_formats) {
			for (auto &format : formats) {
				if (format.format == image_format && format.colorSpace == color_space) {
					return format;
				}
			}
		}
		return Error(ErrorType::VULKAN, "Could not find available format");
	}

	util::Result<VkPresentModeKHR, Error> Window::_select_present_mode() {
		uint32_t avail_count = 0;
		if (auto err = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &avail_count, nullptr)) {
			return Error(ErrorType::VULKAN, "Could not query present modes", VkError(err));
		}
		auto modes = std::vector<VkPresentModeKHR>(avail_count);
		if (auto err = vkGetPhysicalDeviceSurfacePresentModesKHR(_physical_device, _surface, &avail_count, modes.data())) {
			return Error(ErrorType::VULKAN, "Could not query present modes", VkError(err));
		}

		for (auto &mode : modes) {
			if (mode == VK_PRESENT_MODE_FIFO_KHR) {
				return mode;
			}
		}

		return Error(ErrorType::VULKAN, "Could not find available format");
	}

	util::Result<void, Error> Window::_recreate_window() {
		auto device = vulkan::Graphics::DEFAULT->device();
		auto surface_format = VkSurfaceFormatKHR{};
		if (auto err = _select_surface_format().move_or(surface_format)) {
			return Error(ErrorType::VULKAN, "Could not select surface format when recreating window", *err);
		}

		auto present_mode = VkPresentModeKHR{};
		if (auto err = _select_present_mode().move_or(present_mode)) {
			return Error(ErrorType::VULKAN, "Could not select present mode when recreating window", *err);
		}

		for (auto &f : _frames) {
			f = {};
		}

		{
			auto old_swapchain = _swapchain;

			VkSwapchainCreateInfoKHR info = {};
			info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			info.surface = _surface;
			info.minImageCount = IMAGE_COUNT;
			info.imageColorSpace = surface_format.colorSpace;
			info.imageArrayLayers = 1;
			info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
			info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			info.presentMode = present_mode;
			info.clipped = VK_TRUE;
			info.oldSwapchain = old_swapchain;

			auto capabilities = VkSurfaceCapabilitiesKHR();
			util::require(
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physical_device, _surface, &capabilities),
				"Failed to get surface capabilities"
			);

			if (info.minImageCount < capabilities.minImageCount) {
				info.minImageCount = capabilities.minImageCount;
			} else if (
				capabilities.maxImageCount != 0
					&& info.minImageCount > capabilities.maxImageCount
			) {
				info.minImageCount = capabilities.maxImageCount;
			}

			if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
				info.imageExtent.width = _width;
				info.imageExtent.height = _height;
			} else {
				_width = capabilities.currentExtent.width;
				_height = capabilities.currentExtent.height;
			}

			if (auto err = vkCreateSwapchainKHR(device, &info, nullptr, &_swapchain)) {
				return Error(ErrorType::VULKAN, "Could not create swapchain", VkError(err));
			}

			uint32_t image_count = 0;
			if (auto err = vkGetSwapchainImagesKHR(device, _swapchain, &image_count, nullptr)) {
				return Error(ErrorType::VULKAN, "Could not query swapchain image count", VkError(err));
			}
			if (image_count != IMAGE_COUNT) {
				return Error(
					ErrorType::VULKAN,
					std::format("Image count does not match expected count ({})", IMAGE_COUNT)
				);
			}
			
			auto images = std::array<VkImage, IMAGE_COUNT>();
			if (auto err = vkGetSwapchainImagesKHR(device, _swapchain, &image_count, images.data())) {
				return Error(ErrorType::VULKAN, "Could not acquire swapchain images", VkError(err));
			}

			for (auto i = 0; i < IMAGE_COUNT; i++) {
				_frames[i].image = images[i];
			}

			if (old_swapchain) {
				vkDestroySwapchainKHR(device, old_swapchain, nullptr);
			}
		}

		{
			auto attachment = VkAttachmentDescription{};
			attachment.format = surface_format.format;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			// sometimes set to don't care in example code
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			auto color_attachment = VkAttachmentReference{};
			color_attachment.attachment = 0;
			color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			auto subpass = VkSubpassDescription{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &color_attachment;
			auto dependency = VkSubpassDependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;
			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			auto info = VkRenderPassCreateInfo{};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 1;
			info.pAttachments = &attachment;
			info.subpassCount = 1;
			info.pSubpasses = &subpass;
			info.dependencyCount = 1;
			info.pDependencies = &dependency;
			if (auto err = vkCreateRenderPass(device, &info, nullptr, &_render_pass)) {
				return Error(ErrorType::VULKAN, "Could not create render pass", VkError(err));
			}
		}

		{
			auto info = VkImageViewCreateInfo{};
			info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			info.format = surface_format.format;
			info.components.r = VK_COMPONENT_SWIZZLE_R;
			info.components.g = VK_COMPONENT_SWIZZLE_G;
			info.components.b = VK_COMPONENT_SWIZZLE_B;
			info.components.a = VK_COMPONENT_SWIZZLE_A;
			auto image_range = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
			info.subresourceRange = image_range;
			for (uint32_t i = 0; i < IMAGE_COUNT; i++) {
				auto &f = _frames[i];
				info.image = f.image;
				if (auto err = vkCreateImageView(device, &info, nullptr, &f.image_view)) {
					return Error(ErrorType::VULKAN, "Could not create image view", VkError(err));
				}
			}
		}

		{
			auto attachment = VkImageView{};

			auto info = VkFramebufferCreateInfo{};
			info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			info.renderPass = _render_pass;
			info.attachmentCount = 1;
			info.attachmentCount = 1;
			info.pAttachments = &attachment;
			info.width = _width;
			info.height = _height;
			info.layers = 1;

			for (uint32_t i = 0; i < IMAGE_COUNT; i++) {
				attachment = _frames[i].image_view;
				if (auto err = vkCreateFramebuffer(device, &info, nullptr, &_frames[i].framebuffer)) {
					return Error(ErrorType::VULKAN, "Could not create framebuffer", VkError(err));
				}
			}

		}

		for (uint32_t i = 0; i < IMAGE_COUNT; i++) {
			auto &f = _frames[i];
			{
				auto info = VkCommandBufferAllocateInfo{};
				info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				info.commandPool = Graphics::DEFAULT->command_pool();
				info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				info.commandBufferCount = 1;
				if (auto err = vkAllocateCommandBuffers(device, &info, &f.command_buffer)) {
					return Error(
						ErrorType::VULKAN,
						"Could not allocate command buffers",
						VkError(err)
					);
				}
			}

			if (auto err = Fence::create().move_or(f.fence)) {
				return Error(ErrorType::VULKAN, "Could not setup frame fence", VkError(*err));
			}
		}

		for (uint32_t i = 0; i < SEMAPHORE_COUNT; i++) {
			auto semaphore = Semaphore::create(std::format("Window semaphore {}", i));
		}

		return {};
	}
}
