#pragma once

#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan_core.h"
#include <imgui.h>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Graphics;

	class UIRenderPipeline {
		public:
			UIRenderPipeline(Graphics const &graphics);
			~UIRenderPipeline();

			void submit();

		private:
			void createDescriptorPool_();
			void initImGui_();
			void setupVulkanWindow_(int width, int height);

			void renderFrame_(ImDrawData *drawData);
			void presentFrame_();

			Graphics const &graphics_;
			VkDescriptorPool descriptorPool_;
			ImGui_ImplVulkanH_Window windowData_;
			ImGuiIO *io_;
			bool swapchainRebuild_;
			static const int minImageCount_ = 2;
			constexpr static const ImVec4 clearColor_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	};
}