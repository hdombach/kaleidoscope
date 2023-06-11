#include "app.h"
#include "graphics.h"
#include <cstdint>
#include <memory>
#include <sys/types.h>
#include <vector>
#include <iostream>
#define GLFW_INCLUDE_VULKAN

#include <exception>
#include <GLFW/glfw3.h>

int main() {
	auto app = App("Kaleidoscope");

	try {
		app.mainLoop();
	} catch (const std::exception& e) {
		std::cerr << "runtime exception: " << std::endl;
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
