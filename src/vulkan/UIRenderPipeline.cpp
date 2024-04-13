#include "UIRenderPipeline.hpp"
#include "defs.hpp"
#include "error.hpp"
#include "../util/format.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "../util/log.hpp"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <functional>
#include <vulkan/vulkan.h>
#include "graphics.hpp"
#include <array>
#include <imgui.h>

//https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp

namespace vulkan {
	UIRenderPipeline::UIRenderPipeline() {
		_create_descriptor_pool();
		_init_im_gui();
	}

	UIRenderPipeline::~UIRenderPipeline() {
		require(vkDeviceWaitIdle(Graphics::DEFAULT->device()));
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

		ImGui_ImplVulkanH_DestroyWindow(
				Graphics::DEFAULT->instance(),
				Graphics::DEFAULT->device(),
				&_window_data,
				nullptr);
		vkDestroyDescriptorPool(Graphics::DEFAULT->device(), _descriptor_pool, nullptr);
	}

	void UIRenderPipeline::submit(std::function<void()> ui_callback) {
		glfwPollEvents();
		if (_swapchain_rebuild) {
			int width, height;
			glfwGetFramebufferSize(Graphics::DEFAULT->window(), &width, &height);
			if (width > 0 && height > 0) {
				ImGui_ImplVulkan_SetMinImageCount(vulkan::FRAMES_IN_FLIGHT);
				ImGui_ImplVulkanH_CreateOrResizeWindow(
						Graphics::DEFAULT->instance(),
						Graphics::DEFAULT->physicalDevice(),
						Graphics::DEFAULT->device(),
						&_window_data,
						Graphics::DEFAULT->findQueueFamilies().graphicsFamily.value(),
						nullptr,
						width,
						height,
						FRAMES_IN_FLIGHT);
				_window_data.FrameIndex = 0;
				_swapchain_rebuild = false;
			}
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ui_callback();

		ImGui::Render();
		auto mainDrawData = ImGui::GetDrawData();
		const bool mainIsMinimized = (mainDrawData->DisplaySize.x <= 0.0f || mainDrawData->DisplaySize.y <= 0.0f);
		_window_data.ClearValue.color.float32[0] = _clear_color.x * _clear_color.w;
		_window_data.ClearValue.color.float32[1] = _clear_color.y * _clear_color.w;
		_window_data.ClearValue.color.float32[2] = _clear_color.z * _clear_color.w;
		_window_data.ClearValue.color.float32[3] = _clear_color.w;

		if (!mainIsMinimized) {
			_render_frame(mainDrawData);
		}

		if (_io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		if (!mainIsMinimized) {
			_present_frame();
		}

	}

	void UIRenderPipeline::_create_descriptor_pool() {
		auto poolSizes = std::array<VkDescriptorPoolSize, 11>{{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		}};

		auto poolInfo = VkDescriptorPoolCreateInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000 * poolSizes.size();
		poolInfo.poolSizeCount = poolSizes.size();
		poolInfo.pPoolSizes = poolSizes.data();

		require(vkCreateDescriptorPool(Graphics::DEFAULT->device(), &poolInfo, nullptr, &_descriptor_pool));
	}

	void UIRenderPipeline::_init_im_gui() {
		int w, h;
		glfwGetFramebufferSize(Graphics::DEFAULT->window(), &w, &h);
		_setup_vulkan_window(w, h);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		_io = &ImGui::GetIO();
		_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable; //Allows imgui windows to be combined
		_io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // allows imgui windows to be dragged outisde of main window
		_io->Fonts->AddFontFromFileTTF("assets/Helvetica.ttc", 14);

		ImGui::StyleColorsDark();

		auto& style = ImGui::GetStyle();
		style.WindowRounding = 10.0f;
		//style.Colors[ImGuiCol_WindowBg] = ImVec4{0.2f, 0.2f, 0.2f, 1.0f};


		ImGui_ImplGlfw_InitForVulkan(Graphics::DEFAULT->window(), true);
		auto init_info = ImGui_ImplVulkan_InitInfo{};
		init_info.Instance = Graphics::DEFAULT->instance();
		init_info.PhysicalDevice = Graphics::DEFAULT->physicalDevice();
		init_info.Device = Graphics::DEFAULT->device();
		init_info.QueueFamily = Graphics::DEFAULT->findQueueFamilies().graphicsFamily.value();
		init_info.Queue = Graphics::DEFAULT->graphicsQueue();
		init_info.DescriptorPool = _descriptor_pool;
		init_info.Subpass = 0;
		util::log_event(util::f("Min image count ", FRAMES_IN_FLIGHT));
		init_info.MinImageCount = FRAMES_IN_FLIGHT;
		init_info.ImageCount = FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, _window_data.RenderPass);

		Graphics::DEFAULT->executeSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
				ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		});
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void UIRenderPipeline::_setup_vulkan_window(int width, int height) {
		_window_data.Surface = Graphics::DEFAULT->surface();

		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(
				Graphics::DEFAULT->physicalDevice(),
				Graphics::DEFAULT->findQueueFamilies().graphicsFamily.value(),
				Graphics::DEFAULT->surface(),
				&res);
		if (res != VK_TRUE) {
			util::log_error("No WSI support on physical device 0");
		}

    const VkFormat requestSurfaceImageFormat[] = {
			VK_FORMAT_R8G8B8A8_SRGB,
		};
   	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;


		_window_data.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
				Graphics::DEFAULT->physicalDevice(),
				_window_data.Surface,
				requestSurfaceImageFormat,
				(size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
				requestSurfaceColorSpace);

		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };

		_window_data.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
				Graphics::DEFAULT->physicalDevice(),
				_window_data.Surface,
				present_modes,
				IM_ARRAYSIZE(present_modes));

		ImGui_ImplVulkanH_CreateOrResizeWindow(
				Graphics::DEFAULT->instance(),
				Graphics::DEFAULT->physicalDevice(),
				Graphics::DEFAULT->device(),
				&_window_data,
				Graphics::DEFAULT->findQueueFamilies().graphicsFamily.value(),
				nullptr,
				width,
				height,
				FRAMES_IN_FLIGHT);
	}

	void UIRenderPipeline::_render_frame(ImDrawData* drawData) {
		VkResult err;

		auto imageAcquiredSemaphore =_window_data.FrameSemaphores[_window_data.SemaphoreIndex].ImageAcquiredSemaphore;
		auto renderCompleteSemaphore = _window_data.FrameSemaphores[_window_data.SemaphoreIndex].RenderCompleteSemaphore;

		err = vkAcquireNextImageKHR(
				Graphics::DEFAULT->device(),
				_window_data.Swapchain,
				UINT64_MAX,
				imageAcquiredSemaphore,
				VK_NULL_HANDLE,
				&_window_data.FrameIndex);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			_swapchain_rebuild = true;
			return;
		}
		require(err);

		auto frame = &_window_data.Frames[_window_data.FrameIndex];

		require(vkWaitForFences(
					Graphics::DEFAULT->device(),
					1,
					&frame->Fence,
					VK_TRUE, UINT64_MAX));
		require(vkResetFences(Graphics::DEFAULT->device(), 1, &frame->Fence));

		require(vkResetCommandPool(Graphics::DEFAULT->device(), frame->CommandPool, 0));
		auto bufferBeginInfo = VkCommandBufferBeginInfo{};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		require(vkBeginCommandBuffer(frame->CommandBuffer, &bufferBeginInfo));

		auto passBeginInfo = VkRenderPassBeginInfo{};
		passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passBeginInfo.renderPass = _window_data.RenderPass;
		passBeginInfo.framebuffer = frame->Framebuffer;
		passBeginInfo.renderArea.extent.width = _window_data.Width;
		passBeginInfo.renderArea.extent.height = _window_data.Height;
		passBeginInfo.clearValueCount = 1;
		passBeginInfo.pClearValues = &_window_data.ClearValue;
		vkCmdBeginRenderPass(
				frame->CommandBuffer,
				&passBeginInfo,
				VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(drawData, frame->CommandBuffer);

		vkCmdEndRenderPass(frame->CommandBuffer);

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		auto submitInfo = VkSubmitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &imageAcquiredSemaphore;
		submitInfo.pWaitDstStageMask = &waitStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &frame->CommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphore;

		require(vkEndCommandBuffer(frame->CommandBuffer));
		require(vkQueueSubmit(
					Graphics::DEFAULT->graphicsQueue(),
					1,
					&submitInfo,
					frame->Fence));
	}

	void UIRenderPipeline::_present_frame() {
		if (_swapchain_rebuild) {
			return;
		}

		auto renderCompleteSemaphore =
			_window_data.FrameSemaphores[_window_data.SemaphoreIndex].RenderCompleteSemaphore;

		auto presentInfo = VkPresentInfoKHR{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &_window_data.Swapchain;
		presentInfo.pImageIndices = &_window_data.FrameIndex;
		VkResult err = vkQueuePresentKHR(Graphics::DEFAULT->graphicsQueue(), &presentInfo);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			_swapchain_rebuild = true;
			return;
		}
		require(err);
		_window_data.SemaphoreIndex =
			(_window_data.SemaphoreIndex + 1) % _window_data.ImageCount;
	}
}
