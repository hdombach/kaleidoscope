#include "App.hpp"
#include "util/log.hpp"
#include <sys/types.h>
#include <iostream>
#define GLFW_INCLUDE_VULKAN

#include <exception>
#include <GLFW/glfw3.h>

int main(int argc, char **argv) {
	App::set_prog_path(argv[0]);
	auto app = App("Kaleidoscope");

	try {
		app.main_loop();
	} catch (const std::exception& e) {
		std::cerr << "runtime exception: " << std::endl;
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
