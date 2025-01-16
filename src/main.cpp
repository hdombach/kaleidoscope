#include <iostream>
#include <exception>
#include <cstdlib>

#include "App.hpp"
#include "util/Env.hpp"

int main(int argc, char **argv) {
	util::Env::setup(argc, argv);
	auto app = App::create("Kaleidoscope");

	try {
		app->main_loop();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
