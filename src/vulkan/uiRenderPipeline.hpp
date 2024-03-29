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

			void submit(std::function<void()> uiCallback);

		private:
			void createDescriptorPool_();
			void initImGui_();
			void setupVulkanWindow_(int width, int height);

			void renderFrame_(ImDrawData *drawData);
			void presentFrame_();

			VkDescriptorPool descriptorPool_;
			ImGui_ImplVulkanH_Window windowData_;
			ImGuiIO *io_;
			bool swapchainRebuild_;
			constexpr static const ImVec4 clearColor_ = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
	};
}
