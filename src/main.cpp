#include "Defs.h"
#include "Graphics.h"
#include "Window.h"
#include "util/file.h"
#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <vector>
#define GLFW_INCLUDE_VULKAN

#include <exception>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <fstream>

class KaleidoscopeApplication {
	public:
		void run();

	private:
		void drawFrame();

		void mainLoop();
		void cleanup();
		VkShaderModule createShaderModule(const std::vector<char>& code);

		const uint32_t WIDTH = 800;
		const uint32_t HEIGHT = 600;
		vulkan::SharedWindow window;
		vulkan::Graphics graphics;
		vulkan::SharedInstance instance;
		vulkan::SharedDebugUtilsMessenger debugMessenger;
		vulkan::PhysicalDevice physicalDevice;
		vulkan::SharedDevice device;
		vulkan::SharedSurface surface;
		vulkan::SharedSwapchain swapchain;
		vulkan::SharedRenderPass renderPass;
		vulkan::SharedPipeline pipeline;
		std::vector<vulkan::SharedFramebuffer> swapChainFramebuffers;
		vulkan::SharedCommandPool commandPool;
		vulkan::SharedCommandBuffer commandBuffer;
		vulkan::SharedSemaphore imageAvailableSemaphore;
		vulkan::SharedSemaphore renderFinishedSemaphore;
		vulkan::SharedFence inFlightFence;
};

void KaleidoscopeApplication::run() {
	glfwInit();
	window = std::make_shared<vulkan::Window>(vulkan::Window("Kaleidoscope"));
	graphics = vulkan::Graphics(window);
	mainLoop();
	cleanup();
}

void KaleidoscopeApplication::mainLoop() {
	while (!glfwWindowShouldClose(window->raw())) {
		glfwPollEvents();
		graphics.drawFrame();
	}

	graphics.waitIdle();
}

void KaleidoscopeApplication::cleanup() {
	glfwTerminate();
}

int main() {
	KaleidoscopeApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << "runtime exception: " << std::endl;
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
