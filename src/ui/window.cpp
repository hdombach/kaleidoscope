#include "window.h"
#include "format.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "log.h"
#include "vulkan/vulkan_core.h"
#include <vector>

namespace ui {
	void Window::show(VkImageView viewport, vulkan::Graphics const &graphics) {
		auto imguiViewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(imguiViewport->WorkPos);
		ImGui::SetNextWindowSize(imguiViewport->WorkSize);

		ImGui::Begin("Hello World Example");
		ImGui::Text("Hello World");

		static auto textures = std::vector<VkDescriptorSet>{vulkan::Graphics::MIN_IMAGE_COUNT};

		auto texture = ImGui_ImplVulkan_AddTexture(
				graphics.mainTextureSampler(),
				viewport,
				VK_IMAGE_LAYOUT_GENERAL);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		ImGui::Image(texture, ImVec2{viewportPanelSize.x, viewportPanelSize.y});
		ImGui::End();
	}
}
