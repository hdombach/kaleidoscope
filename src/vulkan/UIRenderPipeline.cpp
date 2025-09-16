#include <array>
#include <cstdint>
#include <functional>

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <imgui.h>

#include "UIRenderPipeline.hpp"
#include "defs.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "util/Env.hpp"
#include "util/log.hpp"
#include "graphics.hpp"
#include "util/file.hpp"

//https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp

namespace vulkan {
	UIRenderPipeline::UIRenderPipeline() {
		_create_descriptor_pool();
		_init_im_gui();
		_semaphore = std::move(Semaphore::create().value());
	}

	UIRenderPipeline::~UIRenderPipeline() {
		util::require(vkDeviceWaitIdle(Graphics::DEFAULT->device()));
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

	VkSemaphore UIRenderPipeline::submit(
			std::function<void()> ui_callback,
			VkSemaphore semaphore)
	{
		VkSemaphore result = nullptr;
		glfwPollEvents();
		if (_swapchain_rebuild) {
			int width, height;
			glfwGetFramebufferSize(Graphics::DEFAULT->window(), &width, &height);
			if (width > 0 && height > 0) {
				ImGui_ImplVulkan_SetMinImageCount(vulkan::FRAMES_IN_FLIGHT);
				ImGui_ImplVulkanH_CreateOrResizeWindow(
						Graphics::DEFAULT->instance(),
						Graphics::DEFAULT->physical_device(),
						Graphics::DEFAULT->device(),
						&_window_data,
						Graphics::DEFAULT->find_queue_families().graphics_family.value(),
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
		auto main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
		_window_data.ClearValue.color.float32[0] = _clear_color.x * _clear_color.w;
		_window_data.ClearValue.color.float32[1] = _clear_color.y * _clear_color.w;
		_window_data.ClearValue.color.float32[2] = _clear_color.z * _clear_color.w;
		_window_data.ClearValue.color.float32[3] = _clear_color.w;

		if (!main_is_minimized) {
			result = _render_frame(main_draw_data, semaphore);
		}

		if (_io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		if (!main_is_minimized) {
			_present_frame();
		}

		return result;
	}

	void UIRenderPipeline::_create_descriptor_pool() {
		auto pool_sizes = std::array<VkDescriptorPoolSize, 11>{{
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

		auto pool_info = VkDescriptorPoolCreateInfo{};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * pool_sizes.size();
		pool_info.poolSizeCount = pool_sizes.size();
		pool_info.pPoolSizes = pool_sizes.data();

		util::require(vkCreateDescriptorPool(Graphics::DEFAULT->device(), &pool_info, nullptr, &_descriptor_pool));
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
		log_debug() << "Font path is " << util::g_env.working_dir << "/assets/Helvetica.ttc" << std::endl;
		//TODO: error handling
		_io->Fonts->AddFontFromFileTTF(util::env_file_path("./assets/Helvetica.ttc").value().c_str(), 14);

		/*
		#define ICON_MIN_FA 0xe005
		#define ICON_MAX_FA 0xf8ff

		auto config = ImFontConfig();
		config.MergeMode = true;
		static ImWchar ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		_io->Fonts->AddFontFromFileTTF(util::env_file_path("./assets/NotoEmoji-Medium.ttf").value().c_str(), 14, &config, ranges);
		*/

		ImGui::StyleColorsDark();

		auto& style = ImGui::GetStyle();
		style.WindowRounding = 10.0f;
		//style.Colors[ImGuiCol_WindowBg] = ImVec4{0.2f, 0.2f, 0.2f, 1.0f};


		ImGui_ImplGlfw_InitForVulkan(Graphics::DEFAULT->window(), true);
		auto init_info = ImGui_ImplVulkan_InitInfo{};
		init_info.Instance = Graphics::DEFAULT->instance();
		init_info.PhysicalDevice = Graphics::DEFAULT->physical_device();
		init_info.Device = Graphics::DEFAULT->device();
		init_info.QueueFamily = Graphics::DEFAULT->find_queue_families().graphics_family.value();
		init_info.Queue = Graphics::DEFAULT->graphics_queue();
		init_info.DescriptorPool = _descriptor_pool;
		init_info.Subpass = 0;
		log_event() << "Min image count: " << FRAMES_IN_FLIGHT << std::endl;
		init_info.MinImageCount = FRAMES_IN_FLIGHT;
		init_info.ImageCount = FRAMES_IN_FLIGHT;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, _window_data.RenderPass);

		Graphics::DEFAULT->execute_single_time_command([&](VkCommandBuffer commandBuffer) {
				ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		});
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void UIRenderPipeline::_setup_vulkan_window(int width, int height) {
		_window_data.Surface = Graphics::DEFAULT->surface();

		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(
				Graphics::DEFAULT->physical_device(),
				Graphics::DEFAULT->find_queue_families().graphics_family.value(),
				Graphics::DEFAULT->surface(),
				&res);
		if (res != VK_TRUE) {
			log_error() << "No WSI support on physical device 0" << std::endl;
		}

    const VkFormat requestSurfaceImageFormat[] = {
			VK_FORMAT_R8G8B8A8_SRGB,
		};
   	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;


		_window_data.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
				Graphics::DEFAULT->physical_device(),
				_window_data.Surface,
				requestSurfaceImageFormat,
				(size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
				requestSurfaceColorSpace);

		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };

		_window_data.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
				Graphics::DEFAULT->physical_device(),
				_window_data.Surface,
				present_modes,
				IM_ARRAYSIZE(present_modes));

		ImGui_ImplVulkanH_CreateOrResizeWindow(
				Graphics::DEFAULT->instance(),
				Graphics::DEFAULT->physical_device(),
				Graphics::DEFAULT->device(),
				&_window_data,
				Graphics::DEFAULT->find_queue_families().graphics_family.value(),
				nullptr,
				width,
				height,
				FRAMES_IN_FLIGHT);
	}

	VkSemaphore UIRenderPipeline::_render_frame(
			ImDrawData* drawData,
			VkSemaphore semaphore)
	{
		VkResult err;

		auto image_acquired_semaphore =_window_data.FrameSemaphores[_window_data.SemaphoreIndex].ImageAcquiredSemaphore;
		auto render_complete_semaphore = _window_data.FrameSemaphores[_window_data.SemaphoreIndex].RenderCompleteSemaphore;

		err = vkAcquireNextImageKHR(
				Graphics::DEFAULT->device(),
				_window_data.Swapchain,
				UINT64_MAX,
				image_acquired_semaphore,
				VK_NULL_HANDLE,
				&_window_data.FrameIndex);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			_swapchain_rebuild = true;
			return nullptr;
		}
		util::require(err);

		auto frame = &_window_data.Frames[_window_data.FrameIndex];

		util::require(vkWaitForFences(
					Graphics::DEFAULT->device(),
					1,
					&frame->Fence,
					VK_TRUE, UINT64_MAX));
		util::require(vkResetFences(Graphics::DEFAULT->device(), 1, &frame->Fence));

		util::require(vkResetCommandPool(Graphics::DEFAULT->device(), frame->CommandPool, 0));
		auto buffer_begin_info = VkCommandBufferBeginInfo{};
		buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		buffer_begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		util::require(vkBeginCommandBuffer(frame->CommandBuffer, &buffer_begin_info));

		auto pass_begin_info = VkRenderPassBeginInfo{};
		pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		pass_begin_info.renderPass = _window_data.RenderPass;
		pass_begin_info.framebuffer = frame->Framebuffer;
		pass_begin_info.renderArea.extent.width = _window_data.Width;
		pass_begin_info.renderArea.extent.height = _window_data.Height;
		pass_begin_info.clearValueCount = 1;
		pass_begin_info.pClearValues = &_window_data.ClearValue;
		vkCmdBeginRenderPass(
				frame->CommandBuffer,
				&pass_begin_info,
				VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(drawData, frame->CommandBuffer);

		vkCmdEndRenderPass(frame->CommandBuffer);

		auto wait_semaphores = std::vector<VkSemaphore>();
		auto wait_stages = std::vector<VkPipelineStageFlags>();
		wait_semaphores.push_back(image_acquired_semaphore);
		wait_stages.push_back(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
		if (semaphore) {
			wait_semaphores.push_back(semaphore);
			wait_stages.push_back(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
		}

		auto semaphores = std::array<VkSemaphore,2>{
			render_complete_semaphore,
			_semaphore.get()
		};

		auto submit_info = VkSubmitInfo{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = wait_semaphores.size();
		submit_info.pWaitSemaphores = wait_semaphores.data();
		submit_info.pWaitDstStageMask = wait_stages.data();
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &frame->CommandBuffer;
		submit_info.signalSemaphoreCount = semaphores.size();
		submit_info.pSignalSemaphores = semaphores.data();

		util::require(vkEndCommandBuffer(frame->CommandBuffer));
		util::require(vkQueueSubmit(
					Graphics::DEFAULT->graphics_queue(),
					1,
					&submit_info,
					frame->Fence));

		return _semaphore.get();
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
		VkResult err = vkQueuePresentKHR(Graphics::DEFAULT->graphics_queue(), &presentInfo);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			_swapchain_rebuild = true;
			return;
		}
		util::require(err);
		_window_data.SemaphoreIndex =
			(_window_data.SemaphoreIndex + 1) % _window_data.ImageCount;
	}
}
