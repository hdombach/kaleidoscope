#include <iostream>
#include <exception>
#include <cstdlib>

#include "App.hpp"

int main(int argc, char **argv) {
	App::set_prog_path(argv[0]);
	auto app = App::create("Kaleidoscope");

	try {
		app->main_loop();
	} catch (const std::exception& e) {
		std::cerr << "runtime exception: " << std::endl;
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
