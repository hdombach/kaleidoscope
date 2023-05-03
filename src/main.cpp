#include "graphics.h"
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include <iostream>
#define GLFW_INCLUDE_VULKAN

#include <exception>
#include <GLFW/glfw3.h>


class KaleidoscopeApplication {
	public:
		void run();

	private:
		void drawFrame();

		void mainLoop();
		void cleanup();
		VkShaderModule createShaderModule(const std::vector<char>& code);

		vulkan::Graphics graphics;
};

void KaleidoscopeApplication::run() {
	graphics = vulkan::Graphics("Kaleidoscope");
	mainLoop();
	cleanup();
}

void KaleidoscopeApplication::mainLoop() {
	while (!glfwWindowShouldClose(graphics.window())) {
		glfwPollEvents();
		graphics.tick();
	}

	graphics.waitIdle();
}

void KaleidoscopeApplication::cleanup() {
	graphics.destroy();
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
