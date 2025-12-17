#include <iostream>
#include <exception>
#include <cstdlib>

#include "App.hpp"
#include "util/Env.hpp"
#include "util/ThreadPool.hpp"

int parse_args(int argc, char **argv) {
	if (argc <= 0) {
		return 0;
	}
	if (strcmp(argv[0], "-v") == 0) {
		util::g_log_flags |= util::Importance::DEBUG;
		return 1;
	} else if (strcmp(argv[0], "-vv") == 0) {
		util::g_log_flags |= util::Importance::DEBUG | util::Importance::TRACE;
		return 1;
	} else {
		log_fatal_error() << "Unknown arg: " << argv[0] << std::endl;
		return 1;
	}
}

int main(int argc, char **argv) {
	int i = 1;
	while (i < argc) {
		i += parse_args(argc - i, argv + i);
	}

	try {
		util::Env::setup(argc, argv);
		auto app = App::create("Kaleidoscope");

		app->main_loop();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		ThreadPool::DEFAULT.shutdown();
		return EXIT_FAILURE;
	}

	ThreadPool::DEFAULT.shutdown();
	return EXIT_SUCCESS;
}
