#pragma once

#include <filesystem>

namespace util {
	struct Env {
		int argc;
		char **argv;

		std::filesystem::path prog_path;
		std::filesystem::path working_dir;

		static void setup(int argc, char **argv);
	};

	static Env g_env;

	inline void Env::setup(int argc, char **argv) {
		g_env.argc = argc;
		g_env.argv = argv;

		g_env.prog_path = std::filesystem::path(argv[0]);
		g_env.working_dir = g_env.prog_path.parent_path();
	}
}
