#define GLFW_INCLUDE_VULKAN

#include <exception>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

class KaleidoscopeApplication {
	public:
		void run() {
			initWindow();
			initVulkan();
			main_Loop();
			cleanup();
		}

	private:
		void initWindow() {
			glfwInit();

			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

			window = glfwCreateWindow(WIDTH, HEIGHT, "Kaleidoscope", nullptr, nullptr);
		}

		void initVulkan() {
		}

		void main_Loop() {
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
			}
		}

		void cleanup() {
			glfwDestroyWindow(window);

			glfwTerminate();
		}

		GLFWwindow* window;
		const uint32_t WIDTH = 800;
		const uint32_t HEIGHT = 600;
};

int main() {
	KaleidoscopeApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
