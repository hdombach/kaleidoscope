#include "uiRenderPipeline.h"
#include "error.h"
#include "format.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "log.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <vulkan/vulkan.h>
#include "graphics.h"
#include "window.h"
#include <array>
#include <imgui.h>

namespace vulkan {
	UIRenderPipeline::UIRenderPipeline(Graphics const &graphcs): graphics_(graphcs) {
		createDescriptorPool_();
		initImGui_();
	}

	UIRenderPipeline::~UIRenderPipeline() {
		require(vkDeviceWaitIdle(graphics_.device()));
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

		ImGui_ImplVulkanH_DestroyWindow(
				graphics_.instance(),
				graphics_.device(),
				&windowData_,
				nullptr);
		vkDestroyDescriptorPool(graphics_.device(), descriptorPool_, nullptr);
	}

	void UIRenderPipeline::submit() {
		glfwPollEvents();
		if (swapchainRebuild_) {
			int width, height;
			glfwGetFramebufferSize(graphics_.window(), &width, &height);
			if (width > 0 && height > 0) {
				ImGui_ImplVulkan_SetMinImageCount(Graphics::MIN_IMAGE_COUNT);
				ImGui_ImplVulkanH_CreateOrResizeWindow(
						graphics_.instance(),
						graphics_.physicalDevice(),
						graphics_.device(),
						&windowData_,
						graphics_.findQueueFamilies().graphicsFamily.value(),
						nullptr,
						width,
						height,
						Graphics::MIN_IMAGE_COUNT);
				windowData_.FrameIndex = 0;
				swapchainRebuild_ = false;
			}
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ui::Window::show(graphics_.mainRenderPipeline().getImageView(windowData_.FrameIndex), graphics_);

		ImGui::Render();
		auto mainDrawData = ImGui::GetDrawData();
		const bool mainIsMinimized = (mainDrawData->DisplaySize.x <= 0.0f || mainDrawData->DisplaySize.y <= 0.0f);
		windowData_.ClearValue.color.float32[0] = clearColor_.x * clearColor_.w;
		windowData_.ClearValue.color.float32[1] = clearColor_.y * clearColor_.w;
		windowData_.ClearValue.color.float32[2] = clearColor_.z * clearColor_.w;
		windowData_.ClearValue.color.float32[3] = clearColor_.w;

		if (!mainIsMinimized) {
			renderFrame_(mainDrawData);
		}

		if (io_->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		if (!mainIsMinimized) {
			presentFrame_();
		}

	}

	VkExtent2D UIRenderPipeline::viewportSize() const {
		return VkExtent2D{100, 100};
	}

	void UIRenderPipeline::createDescriptorPool_() {
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

		require(vkCreateDescriptorPool(graphics_.device(), &poolInfo, nullptr, &descriptorPool_));
	}

	void UIRenderPipeline::initImGui_() {
		int w, h;
		glfwGetFramebufferSize(graphics_.window(), &w, &h);
		setupVulkanWindow_(w, h);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		io_ = &ImGui::GetIO();
		io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io_->ConfigFlags |= ImGuiConfigFlags_DockingEnable; //Allows imgui windows to be combined
		//io_->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // allows imgui windows to be dragged outisde of main window

		ImGui::StyleColorsDark();

		auto& style = ImGui::GetStyle();
		if (io_->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForVulkan(graphics_.window(), true);
		auto init_info = ImGui_ImplVulkan_InitInfo{};
		init_info.Instance = graphics_.instance();
		init_info.PhysicalDevice = graphics_.physicalDevice();
		init_info.Device = graphics_.device();
		init_info.QueueFamily = graphics_.findQueueFamilies().graphicsFamily.value();
		init_info.Queue = graphics_.graphicsQueue();
		init_info.DescriptorPool = descriptorPool_;
		init_info.Subpass = 0;
		util::log_event(util::stringConcat("Min image count ", Graphics::MIN_IMAGE_COUNT));
		init_info.MinImageCount = Graphics::MIN_IMAGE_COUNT; //idk if these should be 3 or 2
		init_info.ImageCount = Graphics::MIN_IMAGE_COUNT;
		init_info.ImageCount = windowData_.ImageCount;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info, windowData_.RenderPass);

		graphics_.executeSingleTimeCommand([&](VkCommandBuffer commandBuffer) {
				ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
		});
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	void UIRenderPipeline::setupVulkanWindow_(int width, int height) {
		windowData_.Surface = graphics_.surface();

		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(
				graphics_.physicalDevice(),
				graphics_.findQueueFamilies().graphicsFamily.value(),
				graphics_.surface(),
				&res);
		if (res != VK_TRUE) {
			util::log_error("No WSI support on physical device 0");
		}

    const VkFormat requestSurfaceImageFormat[] = {
			VK_FORMAT_B8G8R8A8_UNORM,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_B8G8R8_UNORM,
			VK_FORMAT_R8G8B8_UNORM
		};
    const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

		windowData_.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
				graphics_.physicalDevice(),
				windowData_.Surface,
				requestSurfaceImageFormat,
				(size_t)IM_ARRAYSIZE(requestSurfaceImageFormat),
				requestSurfaceColorSpace);

		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };

		windowData_.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
				graphics_.physicalDevice(),
				windowData_.Surface,
				present_modes,
				IM_ARRAYSIZE(present_modes));

		ImGui_ImplVulkanH_CreateOrResizeWindow(
				graphics_.instance(),
				graphics_.physicalDevice(),
				graphics_.device(),
				&windowData_,
				graphics_.findQueueFamilies().graphicsFamily.value(),
				nullptr,
				width,
				height,
				Graphics::MIN_IMAGE_COUNT);
	}

	void UIRenderPipeline::renderFrame_(ImDrawData* drawData) {
		VkResult err;

		auto imageAcquiredSemaphore =windowData_.FrameSemaphores[windowData_.SemaphoreIndex].ImageAcquiredSemaphore;
		auto renderCompleteSemaphore = windowData_.FrameSemaphores[windowData_.SemaphoreIndex].RenderCompleteSemaphore;

		err = vkAcquireNextImageKHR(
				graphics_.device(),
				windowData_.Swapchain,
				UINT64_MAX,
				imageAcquiredSemaphore,
				VK_NULL_HANDLE,
				&windowData_.FrameIndex);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			swapchainRebuild_ = true;
			return;
		}
		require(err);

		auto frame = &windowData_.Frames[windowData_.FrameIndex];

		require(vkWaitForFences(
					graphics_.device(),
					1,
					&frame->Fence,
					VK_TRUE, UINT64_MAX));
		require(vkResetFences(graphics_.device(), 1, &frame->Fence));

		require(vkResetCommandPool(graphics_.device(), frame->CommandPool, 0));
		auto bufferBeginInfo = VkCommandBufferBeginInfo{};
		bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		require(vkBeginCommandBuffer(frame->CommandBuffer, &bufferBeginInfo));

		auto passBeginInfo = VkRenderPassBeginInfo{};
		passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		passBeginInfo.renderPass = windowData_.RenderPass;
		passBeginInfo.framebuffer = frame->Framebuffer;
		passBeginInfo.renderArea.extent.width = windowData_.Width;
		passBeginInfo.renderArea.extent.height = windowData_.Height;
		passBeginInfo.clearValueCount = 1;
		passBeginInfo.pClearValues = &windowData_.ClearValue;
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
					graphics_.graphicsQueue(),
					1,
					&submitInfo,
					frame->Fence));
	}

	void UIRenderPipeline::presentFrame_() {
		if (swapchainRebuild_) {
			return;
		}

		auto renderCompleteSemaphore =
			windowData_.FrameSemaphores[windowData_.SemaphoreIndex].RenderCompleteSemaphore;

		auto presentInfo = VkPresentInfoKHR{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &windowData_.Swapchain;
		presentInfo.pImageIndices = &windowData_.FrameIndex;
		VkResult err = vkQueuePresentKHR(graphics_.graphicsQueue(), &presentInfo);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
			swapchainRebuild_ = true;
			return;
		}
		require(err);
		windowData_.SemaphoreIndex =
			(windowData_.SemaphoreIndex + 1) % windowData_.ImageCount;
	}
}
